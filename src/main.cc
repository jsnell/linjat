#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <gflags/gflags.h>
#include <map>
#include <set>
#include <vector>

using std::string;

#if !defined(MAP_WIDTH)
#error "MAP_WIDTH undefined"
#endif
#if !defined(MAP_HEIGHT)
#error "MAP_HEIGHT undefined"
#endif
#if !defined(PIECES)
#error "PIECES undefined"
#endif

DEFINE_int32(seed, 1, "RNG seed.");
DEFINE_int32(puzzle_count, 100, "Number of puzzles to generate.");
DEFINE_int32(optimize_iterations, 10000,
             "Number of iterations done during optimization.");

DEFINE_string(solve, "",
              "Solve the puzzle defined in this flag.");
DEFINE_string(solve_progress_file, "",
              "Print solver state after each iteration in this file.");
DEFINE_string(candidate_progress_file, "",
              "Print solver state after each candidate-generation "
              "iteration in this file.");

DEFINE_int32(score_cover, 1,
             "Weight given to basic covering deduction rule.");
DEFINE_int32(score_cant_fit, 1,
             "Weight given to basic piece fit deduction rule.");
DEFINE_int32(score_square, 1,
             "Weight given to forced corners of rectangle "
             "deduction rule.");
DEFINE_int32(score_dep, 1,
             "Weight given to dependent squares deduction rule.");
DEFINE_int32(score_one_of, 1,
             "Weight given to dependent squares deduction rule.");
DEFINE_int32(score_max_width, -1,
             "Weight given to width of solution tree.");
DEFINE_int32(score_single_solution, -50,
             "Weight given to being able to 'cheat' by knowing the "
             "puzzle has exactly one solution.");
DEFINE_int32(score_uncontested_no_cover, -1,
             "Weight given to being able to 'cheat' by knowing the "
             "puzzle has exactly one solution.");

enum DeductionKind {
    NONE = 0,
    COVER = 1,
    CANT_FIT = 2,
    SQUARE = 3,
    DEPENDENCY = 4,
    ONE_OF = 5,
    SINGLE_SOLUTION = 6,
    UNCONTESTED_NO_COVER = 7,
};

class Game {
public:
    static const int W = MAP_WIDTH + 1, H = MAP_HEIGHT, N = PIECES;

    using Hint = std::pair<uint16_t, uint16_t>;
    using Mask = uint64_t;
    using MaskArray = std::array<Mask, H * W>;
    using DepMask = std::bitset<H * W>;

    using IterationResult = std::pair<DeductionKind, int>;

    Game(std::string puzzle = "") {
        for (int r = 0; r < H; ++r) {
            border_[r * W] = 1;
        }
        if (!puzzle.empty()) {
            setup_map(puzzle);
        } else {
            randomize();
        }
        reset_hints();
        reset_possible();
        update_possible();
        orig_possible_ = possible_;
    }

    void randomize() {
        for (int i = 0; i < N; ++i) {
            while (1) {
                int at = random() % (W * H);
                int val = 2 + random() % 4;
                if (!fixed_[at] && !border_[at]) {
                    hints_[i] = Hint(at, val);
                    fixed_[at] = piece_mask(i);
                    break;
                }
            }
        }
    }

    void setup_map(const std::string& map) {
        assert(map.size() == W * H);

        int pieces = 0;
        for (int i = 0; i < H * W; ++i) {
            if (map[i] == ',') {
                assert(border_[i]);
            } else if (map[i] == '.') {
                forced_[i] = true;
            } else if (isdigit(map[i])) {
                int val = map[i] - '0';
                hints_[pieces] = Hint(i, val);
                fixed_[i] = piece_mask(pieces++);
            }
        }

        assert(pieces == N);
    }

    void reset_hints() {
        for (int at = 0; at < W * H; ++at) {
            fixed_[at] = 0;
        }
        for (int i = 0; i < N; ++i) {
            auto& hint = hints_[i];
            fixed_[hint.first] = piece_mask(i);
        }
        for (int i = 0; i < N; ++i) {
            valid_orientation_[i] = init_valid_orientations(i);
        }
    }

    void print_puzzle(bool json) {
        std::map<int, int> hints;
        for (auto hint : hints_) {
            hints[hint.first] = hint.second;
        }

        int at = 0;
        for (int r = 0; r < H; ++r) {
            if (json) {
                printf("\"");
            }
            for (int c = 0; c < W; ++c) {
                if (!border_[at]) {
                    if (hints.count(at)) {
                        printf("%d", hints[at]);
                    } else if (forced_[at]) {
                        printf(".");
                    } else {
                        printf(" ");
                    }
                }
                ++at;
            }
            if (json) {
                printf("\"");
                if (r != H - 1) {
                    printf(", ");
                }
            } else {
                printf("\n");
            }
        }
    }

    void print_fixed() {
        int at = 0;
        for (int r = 0; r < H; ++r) {
            for (int c = 0; c < W; ++c) {
                if (!border_[at]) {
                    if (fixed_[at]) {
                        fprintf(stderr,
                                "%d",
                                hints_[__builtin_ctz(fixed_[at])].second);;
                    } else if (forced_[at]) {
                        fprintf(stderr, ".");
                    } else if (!possible_[at]) {
                        fprintf(stderr, "_");
                    } else {
                        fprintf(stderr, " ");
                    }
                }
                ++at;
            }
            fprintf(stderr, "|\n");
        }
        fprintf(stderr, "\n");
    }

    void print_possible() {
        int at = 0;
        for (int r = 0; r < H; ++r) {
            for (int c = 0; c < W; ++c) {
                // if (!border_[at]) {
                //     printf("%d ",
                //            fixed_[at] ?
                //            hints_[__builtin_ctz(fixed_[at])].second :
                //            0);
                // }
                if (!border_[at]) {
                    if (false) {
                        printf("[%d %d] ",
                               possible_count(at),
                               __builtin_popcountl(orig_possible_[at]) -
                               possible_count(at));
                    } else if (possible_count(at)) {
                        printf("% 2d%c%d ",
                               possible_count(at),
                               forced_[at] ? 'X' : '?',
                               fixed_[at] ?
                               hints_[mask_to_piece(fixed_[at])].second :
                               0);
                    } else {
                        bool printed = false;
                        for (int p = 0; p < N; ++p) {
                            if (hints_[p].first == at) {
                                printf(" % 3d ",
                                       // valid_orientation_[p]);
                                       orientation_count(p));
                                printed = true;
                            }
                        }
                        if (!printed)
                            printf("  .  ");
                    }
                }
                ++at;
            }
            printf("\n");
        }
    }

    void print_json(FILE* f, const char* extra_json) {
        fprintf(f, "{%s\"height\":%d, \"width\":%d, \"lines\":[\n",
                extra_json, H, W -1);
        for (int piece = 0; piece < N; ++piece) {
            int at = hints_[piece].first;
            int value = hints_[piece].second;
            int r = at / W, c = at % W;
            int minr = r, maxr = r, minc = c, maxc = c;
            for (int at = 0; at < W * H; ++at) {
                if (fixed_[at] == piece_mask(piece)) {
                    minr = std::min(minr, at / W);
                    maxr = std::max(maxr, at / W);
                    minc = std::min(minc, at % W);
                    maxc = std::max(maxc, at % W);
                }
            }

            int valid_o = valid_orientation_[piece];
            bool have_vertical = false, have_horizontal = false;
            for (int o = 0; o < value * 2; ++o) {
                if (valid_o & (1 << o)) {
                    if (o & 1) {
                        have_vertical = true;
                    } else {
                        have_horizontal = true;
                    }
                }
            }

            fprintf(f,
                    "{\"r\":%d,\"c\":%d,\"value\":\"%d\",",
                    r, c - 1, value);
            if (have_vertical != have_horizontal) {
                fprintf(f,
                        "\"hz\":%d,"
                        "\"minr\":%d,\"minc\":%d,\"maxr\":%d,\"maxc\":%d,\n",
                        have_horizontal,
                        minr, minc - 1,
                        maxr, maxc - 1);
            }
            fprintf(f,
                    "},\n");
        }
        fprintf(f, "],\"dots\":[\n");
        for (int at = 0; at < W * H; ++at) {
            if (forced_[at]) {
                fprintf(f,
                        "{\"r\":%d,\"c\":%d},\n", at / W, at % W - 1);
            }
        }
        fprintf(f, "],\n");
        fprintf(f, "},\n");
    }

    void reset_possible() {
        for (int at = 0; at < W * H; ++at) {
            possible_[at] = 0;
        }
    }

    IterationResult iterate() {
        reset_possible();

        {
            update_possible();
            int count;

            count = update_uncontested_no_cover();
            if (count) {
                return { DeductionKind::UNCONTESTED_NO_COVER, count };
            }

            count = update_forced_coverage();
            if (count) {
                return { DeductionKind::COVER, count };
            }

            // Put this after forced_coverage, since it's not interesting
            // if abusing knowledge of a single solution gives just the
            // same deduction as the trivial rule.
            count = update_knowledge_of_single_solution();
            if (count) {
                update_forced_coverage();
                return { DeductionKind::SINGLE_SOLUTION, count };
            }

            count = update_cant_fit();
            if (count) {
                return { DeductionKind::CANT_FIT, count };
            }
        }


        {
            update_square();
            // Is there really no need to check u_forced_coverage
            // here?
            int count = update_cant_fit();
            if (count) {
                return { DeductionKind::SQUARE, count };
            }
        }

        {
            update_dependent();
            int count = update_cant_fit();
            if (count) {
                return { DeductionKind::DEPENDENCY, count };
            }
        }

        {
            update_one_of();
            int count = update_cant_fit();
            if (count) {
                return { DeductionKind::ONE_OF, count };
            }
        }

        return { DeductionKind::NONE, 0 };
    }

    int update_cant_fit() {
        int count = 0;
        for (int piece = 0; piece < N; ++piece) {
            count += update_cant_fit_for_piece(piece);
        }
        return count;
    }

    int update_forced_coverage() {
        int count = 0;
        for (int piece = 0; piece < N; ++piece) {
            count += update_forced_coverage_for_piece(piece);
        }
        return count;
    }

    void reset_forced() {
        for (int i = 0; i < W * H; ++i) {
            forced_[i] = false;
        }
    }

    bool force_one_square(int possible_mask = (1 << N) - 1) {
        int best_at = -1;
        int best_score = -1;
        int best_score_count;

        for (int at = 0; at < W *H; ++at) {
            if (fixed_[at] || forced_[at] ||
                !(possible_[at] & possible_mask)) {
                continue;
            }
            int score = orig_possible_count(at) + possible_count(at);
            if (score > best_score) {
                best_score = score;
                best_at = at;
                best_score_count = 1;
            } else if (best_score == score) {
                ++best_score_count;
                if (rand() % best_score_count == 0) {
                    best_at = at;
                }
            }
        }

        if (best_at >= 0) {
            forced_[best_at] = true;
            return true;
        }
        return false;
    }

    bool force_if_uncontested() {
        reset_possible();
        update_possible();
        bool ret = false;
        for (int piece = 0; piece < N; ++piece) {
            if (!find_uncontested_no_cover(piece))
                continue;
            assert(force_one_square(piece_mask(piece)));
            ret = true;
        }

        return ret;
    }

    bool impossible() {
        for (int piece = 0; piece < N; ++piece) {
            if (!valid_orientation_[piece])
                return true;
        }

        for (int at = 0; at < W *H; ++at) {
            if (forced_[at] && !possible_[at]) {
                return true;
            }
        }

        return false;
    }

    bool solved() {
        for (int piece = 0; piece < N; ++piece) {
            if (orientation_count(piece) != 1) {
                return false;
            }
        }

        for (int at = 0; at < W *H; ++at) {
            if (forced_[at] && !fixed_[at]) {
                return false;
            }
        }

        return true;
    }

    void mutate() {
        do {
            int piece = rand() % Game::N;
            int at = hints_[piece].first;
            int size = hints_[piece].second;

            switch (rand() % 3) {
            case 0:
                if (size > 1)
                    hints_[piece].second--;
                break;
            case 1:
                if (size < 8)
                    hints_[piece].second++;
                break;
            case 2:
                fixed_[at] = 0;
                while (1) {
                    int at = random() % (W * H);
                    if (!fixed_[at] && !border_[at]) {
                        hints_[piece].first = at;
                        fixed_[at] = piece_mask(piece);
                        break;
                    }
                }
                break;
            default:
                break;
            }
        } while (rand() % 3 < 1);

        reset_hints();
        reset_possible();
        update_possible();
        orig_possible_ = possible_;
    }

private:
    int orig_possible_count(int at) {
        return __builtin_popcountl(orig_possible_[at]);
    }

    int possible_count(int at) {
        return __builtin_popcountl(possible_[at]);
    }

    Mask piece_mask(uint64_t piece) {
        return UINT64_C(1) << piece;
    }

    int mask_to_piece(Mask mask) {
        return __builtin_ctzl(mask);
    }

    int orientation_count(int piece) {
        return __builtin_popcountl(valid_orientation_[piece]);
    }

    void update_possible() {
        for (int piece = 0; piece < N; ++piece) {
            update_possible_for_piece(piece);
        }
    }

    void update_possible_for_piece(int piece) {
        Mask mask = piece_mask(piece);
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                int count = 0;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (!fixed_[at] || fixed_[at] == mask) {
                        ++count;
                    }
                }
                if (count == size) {
                    for (int at : PieceOrientationIterator(hints_[piece], o)) {
                        possible_[at] |= mask;
                    }
                } else {
                    valid_orientation_[piece] &= ~(1 << o);
                }
            }
        }
    }

    int update_forced_coverage_for_piece(int piece) {
        Mask mask = piece_mask(piece);
        int updated = 0;

        for (int at : PieceIterator(hints_[piece])) {
            if (!fixed_[at]) {
                if ((forced_[at] && possible_[at] == mask)) {
                    updated = 1;
                    fixed_[at] = mask;
                    update_not_possible(at, mask, piece);
                }
            }
        }

        return updated;
    }

    int update_cant_fit_for_piece(int piece) {
        Mask mask = piece_mask(piece);
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        int valid_count = 0;
        std::array<uint8_t, W*H> count = { 0 };

        if (!valid_o) {
            return 0;
        }

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                bool ok = true;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (!(possible_[at] & mask)) {
                        ok = false;
                    }
                }
                if (ok) {
                    ++valid_count;
                    for (int at : PieceOrientationIterator(hints_[piece], o)) {
                        count[at]++;
                    }
                }
            }
        }

        int updated = 0;
        for (int at : PieceIterator(hints_[piece])) {
            if (!fixed_[at]) {
                if (count[at] == valid_count) {
                    updated = 1;
                    fixed_[at] = mask;
                    update_not_possible(at, mask, piece);
                }
            }
        }

        return updated;
    }

    void update_not_possible(int update_at, Mask mask, int piece) {
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                bool no_intersect = true;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (at == update_at) {
                        no_intersect = false;
                    }
                }
                if (no_intersect) {
                    valid_orientation_[piece] &= ~(1 << o);
                }
            }
        }
    }

    int init_valid_orientations(int piece) {
        int size = hints_[piece].second;
        int ret = 0;
        for (int o = 0; o < size * 2; ++o) {
            int count = 0;
            for (int at : PieceOrientationIterator(hints_[piece], o)) {
                if (!border_[at] &&
                    (!fixed_[at] || fixed_[at] == piece_mask(piece))) {
                    ++count;
                }
            }
            bool fit2 = (count == size);
            if (fit2) {
                ret |= (1 << o);
            }
        }
        return ret;
    }

    class PieceOrientationIterator {
    public:
        PieceOrientationIterator(Hint piece, int orientation) {
            int at = piece.first;
            int size = piece.second;
            int offset = (size - 1) - (orientation >> 1);

            step_ = ((orientation & 1) ? W : 1);
            start_ = at - offset * step_;
            end_ = start_ + size * step_;

            if (start_ < 0 || end_ > W * H + step_) {
                start_ = end_ = -1;
            }
        }

        struct iterator {
            iterator(int i, int step) : i_(i), step_(step) {
            }

            bool operator!=(const iterator& other) const {
                return i_ != other.i_;
            }

            int operator*() {
                return i_;
            }

            int operator++() {
                i_ += step_;
                return i_;
            }

            int i_, step_;
        };

        iterator begin() {
            return iterator(start_, step_);
        }

        iterator end() {
            return iterator(end_, step_);
        }

    private:
        int start_, end_, step_;
    };

    class PieceIterator {
    public:
        using Indices = std::array<int, 1 + 2*9 + 2*9>;

        PieceIterator(Hint piece) {
            int at = piece.first;
            int size = piece.second;
            is_[size_++] = at;

            int r = at / W;
            int c = at % W;
            // The column.
            for (int ri = std::max(0, r - (size - 1));
                 ri < std::min(H, r + size);
                 ++ri) {
                int at2 = ri * W + c;
                if (at2 != at)
                    is_[size_++] = at2;
            }
            // The row.
            for (int ci = std::max(0, c - (size - 1));
                 ci < std::min(W, c + size);
                 ++ci) {
                int at2 = ci + r * W;
                if (at2 != at)
                    is_[size_++] = at2;
            }
        }

        struct iterator {
            iterator(int i, Indices* is) : i_(i), is_(is) {
            }

            bool operator!=(const iterator& other) const {
                return i_ != other.i_ || is_ != other.is_;
            }

            int operator*() {
                return (*is_)[i_];
            }

            void operator++() {
                i_++;
            }

            int i_;
            Indices* is_;
        };

        iterator begin() {
            return iterator(0, &is_);
        }

        iterator end() {
            return iterator(size_, &is_);
        }

    private:
        int size_ = 0;
        Indices is_;
    };

    int update_uncontested_no_cover() {
        int count = 0;
        for (int piece = 0; piece < N; ++piece) {
            int omask = find_uncontested_no_cover(piece);
            if (!omask)
                continue;
            // Found a viable uncontested orientation for the piece.
            valid_orientation_[piece] = omask;
            {
                int o = __builtin_ctzl(omask);
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (!fixed_[at]) {
                        fixed_[at] = piece_mask(piece);
                        update_not_possible(at,
                                            piece_mask(piece),
                                            piece);
                    }
                }
            }
            return 1;
        }
        return count;
    }

    int find_uncontested_no_cover(int piece) {
        // If a piece can't be used to cover any more dots, see if there
        // are any totally uncontested orientations. If there is one,
        // just choose it as the actual orientation.

        Mask mask = piece_mask(piece);
        bool have_contested = false;

        // Bail out early if the piece can still be used to cover a
        // dot.
        for (int at : PieceIterator(hints_[piece])) {
            if (!fixed_[at] && forced_[at] &&
                (possible_[at] & mask)) {
                return 0;
            }
            if ((possible_[at] & mask) && possible_[at] != mask) {
                have_contested = true;
            }
        }

        // If none of the orientations are contested, this is an
        // uninteresting case.
        if (!have_contested) {
            return 0;
        }

        // Iterate through all orientations of the piece. Look how many
        // match orientation A from the intro, how many B/C.
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                // In this orientation either all squares are already
                // covered by this piece, or can only be covered by
                // this piece.
                bool ok = true;
                bool non_fixed = false;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (fixed_[at]) {
                        if (fixed_[at] != mask)
                            ok = false;
                    } else {
                        non_fixed = true;
                        if (possible_[at] != mask)
                            ok = false;
                    }
                }
                if (ok && non_fixed) {
                    return 1 << o;
                }
            }
        }

        return 0;
    }

    int update_knowledge_of_single_solution() {
        int count = 0;
        for (int piece = 0; piece < N; ++piece) {
            count += find_knowledge_of_single_solution(piece);
        }
        return count;
    }

    int find_knowledge_of_single_solution(int piece) {
        // Given a piece P, split the valid orientations into two
        // groups.  Those where P overlaps with either a dot or a
        // valid orientation of some other piece (have information),
        // and those where it doesn't (no information).
        //
        // If there are multiple "no information" orientations, a
        // level generator using a single solution can't possibly
        // distinguish between them. Any squares covered by all of the
        // "have information" orientations must therefore be part
        // of the solution.

        DepMask have_information_union;
        int have_information_count = 0;
        int no_information_count = 0;

        // Iterate through all orientations of the piece. Look how many
        // match orientation A from the intro, how many B/C.
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                DepMask covered;
                bool ok = true;
                bool overlaps_forced = false;
                bool cant_overlap_with_other_pieces = true;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    covered.set(at);
                    if (!fixed_[at]) {
                        if (forced_[at])
                            overlaps_forced = true;
                        if (possible_count(at) != 1)
                            cant_overlap_with_other_pieces = false;
                    } else if (fixed_[at] != piece_mask(piece)) {
                        ok = false;
                    }
                }
                if (!ok) {
                    continue;
                }
                if (overlaps_forced) {
                    if (have_information_count++) {
                        have_information_union &= covered;
                    } else {
                        have_information_union = covered;
                    }
                } else if (!overlaps_forced &&
                           cant_overlap_with_other_pieces) {
                    no_information_count++;
                }
            }
        }

        if (no_information_count > 1) {
            int update_count = 0;
            for (int at : PieceIterator(hints_[piece])) {
                if (fixed_[at])
                    continue;
                if (have_information_union[at]) {
                    fixed_[at] = piece_mask(piece);
                    update_not_possible(at, piece_mask(piece), piece);
                    ++update_count;
                }
             }
            return update_count;
        }
        return 0;
    }

    // Find dependencies. E.g.:
    //
    //   53.. 4
    //    5 2
    //
    // Above any row that covers the left dot must also cover the right
    // dot. So the 2 can't go up to cover the right dot.
    void update_dependent() {
        for (int at = 0; at < W * H; ++at) {
            if (!forced_[at])
                continue;
            if (fixed_[at])
                continue;
            if (possible_count(at) <= 1)
                continue;
            DepMask dep;
            dep.set();
            for (int piece = 0; piece < N; ++piece) {
                if (!(piece_mask(piece) & possible_[at]))
                    continue;
                dep &= find_dependent(piece, at);
            }
            if (dep.count() > 1) {
                for (int target = 0; target < N; ++target) {
                    if (dep[target] &&
                        target != at &&
                        possible_[at] != possible_[target]) {
                        if (forced_[target]) {
                            possible_[target] = possible_[at] =
                                (possible_[target] & possible_[at]);
                        }
                    }
                }
            }
        }
    }

    DepMask find_one_of(int piece, int target) {
        return find_dependent(piece, target, false);
    }

    DepMask find_dependent(int piece, int target,
                           bool wanted_overlap = true) {
        Mask mask = piece_mask(piece);
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        bool ret_valid;
        DepMask ret;
        ret.set();

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                bool overlaps_target = false;
                bool ok = true;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (at == target)
                        overlaps_target = true;
                    if (fixed_[at] && fixed_[at] != mask)
                        ok = false;
                }
                if (overlaps_target == wanted_overlap && ok) {
                    DepMask mask;
                    for (int at : PieceOrientationIterator(hints_[piece], o)) {
                        mask[at] = true;
                    }
                    ret &= mask;
                    ret_valid = true;
                }
            }
        }

        if (!ret_valid)
            ret.reset();

        return ret;
    }

    //      Y   5
    //    X 4   Z
    //
    // 4 needs to cover either Y or Z, so can't cover X.
    void update_square() {
        for (int piece = 0; piece < N; ++piece) {
            int size = hints_[piece].second;
            std::vector<int> covered;
            for (int at : PieceIterator(hints_[piece])) {
                if (!forced_[at] || fixed_[at] ||
                    possible_count(at) != 2)
                    continue;
                if (!(possible_[at] & piece_mask(piece)))
                    continue;
                covered.push_back(at);
            }
            // Find pairs of squares where:
            // - Both can be covered by the candidate and one other
            //   piece (the same in both cases).
            // - The candidate piece can't cover both squares at the same
            //   time. (That will implicitly mean that the other piece
            //   can't do it either).
            for (int i = 0; i < covered.size(); ++i) {
                for (int j = i + 1; j < covered.size(); ++j) {
                    int ai = covered[i], aj = covered[j];
                    if (distance(ai, aj) <= size)
                        continue;
                    Mask pi = possible_[ai], pj = possible_[aj];
                    if (pi != pj)
                        continue;
                    // Remove any orientations of the piece that
                    // don't cover at least one of the two squares.
                    int valid_o = valid_orientation_[piece];
                    for (int o = 0; o < size * 2; ++o) {
                        if (valid_o & (1 << o)) {
                            bool no_candidate = true;
                            for (int at :
                                 PieceOrientationIterator(hints_[piece], o)) {
                                if (at == ai || at == aj)
                                    no_candidate = false;
                            }
                            if (no_candidate) {
                                valid_orientation_[piece] &= ~(1 << o);
                            }
                        }
                    }
                }
            }
        }
    }

    // Find dependencies. E.g.:
    //
    //----
    //  y2
    //  3.
    //  Y2
    // ---
    //
    // Both 2s can't be vertical, so either Y or y must be
    // filled. So 3 must be horizontal.
    void update_one_of() {
        for (int at = 0; at < W * H; ++at) {
            // Find squares where two pieces on the same
            // row / column can intersect.
            if (fixed_[at])
                continue;
            if (possible_count(at) < 2)
                continue;
            int piece_a = 0, piece_b = 0;
            // This is slightly suboptimal that it'll only
            // find one pair of potential a / b piece, not
            // pairs.
            if (!find_pieces_on_same_row_or_column(at,
                                                   &piece_a,
                                                   &piece_b))
                continue;
            // For each of those two pieces, find the set of squares
            // that they must pass through if they don't go through
            // the intersecting square.
            DepMask a = find_one_of(piece_a, at);
            DepMask b = find_one_of(piece_b, at);
            Mask target_pieces_a = 0;
            Mask target_pieces_b = 0;
            // Then see if there exists a piece that could
            // intersect with both of the above sets.
            for (int target = 0; target < W * H; ++target) {
                if (!a[target] && !b[target])
                    continue;
                for (int piece = 0; piece < N; ++piece) {
                    if (piece == piece_a || piece == piece_b)
                        continue;
                    if (piece_mask(piece) & possible_[target]) {
                        if (a[target])
                            target_pieces_a |= piece_mask(piece);
                        if (b[target])
                            target_pieces_b |= piece_mask(piece);
                    }
                }
            }
            Mask target_pieces_both = target_pieces_a &
                target_pieces_b;
            if (target_pieces_both) {
                // If such a piece exists, check if the piece
                // can intersect with both sets at the same time.
                // If it can, exclude any such orientations.
                for (int piece = 0; piece < N; ++piece) {
                    if (piece_mask(piece) & target_pieces_both) {
                        exclude_if_in_both_sets(piece, a, b);
                    }
                }
            }
        }
    }

    bool find_pieces_on_same_row_or_column(int at, int* a, int* b) {
        int possible = possible_[at];
        for (int i = 0; i < N; ++i) {
            if (!(possible & piece_mask(i)))
                continue;
            for (int j = i + 1; j < N; ++j) {
                if (!(possible & piece_mask(j)))
                    continue;
                int a_at = hints_[i].first;
                int b_at = hints_[j].first;
                if ((a_at / W == b_at / W) ||
                    (a_at % W == b_at % W)) {
                    *a = i;
                    *b = j;
                    return true;
                }
            }
        }

        return false;
    }

    void exclude_if_in_both_sets(int piece,
                                 const DepMask& a,
                                 const DepMask& b) {
        Mask mask = piece_mask(piece);
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                bool a_hit = false, b_hit = false;
                bool ok = true;
                for (int at : PieceOrientationIterator(hints_[piece], o)) {
                    if (a[at]) a_hit = true;
                    if (b[at]) b_hit = true;
                    if (fixed_[at] && fixed_[at] != mask) {
                        ok = false;
                    }
                }
                if (ok && a_hit && b_hit) {
                    valid_orientation_[piece] &= ~(1 << o);
                }
            }
        }
    }

    int distance(int a, int b) {
        int ra = a / W, rb = b / W,
            ca = a % W, cb = b % W;
        int rd = std::abs(ra - rb), cd = std::abs(ca - cb);
        if (rd && cd) {
            return W + H + 1;
        }
        return rd + cd;
    }

    std::array<Hint, N> hints_;
    uint16_t valid_orientation_[N] { 0 };
    MaskArray orig_possible_;
    MaskArray possible_ = { 0 };
    MaskArray fixed_ = { 0 };
    bool forced_[H * W] = { 0 };
    bool border_[H * W] = { 0 };
};

Game add_forced_squares(Game game, FILE* fp) {
    if (fp)
        game.print_json(fp, "");
    for (int i = 0; i < 100; ++i) {
        if (game.force_if_uncontested() && fp)
            game.print_json(fp, "\"type\":\"ambiguate\",");
        auto res = game.iterate();
        if (res.first == DeductionKind::NONE) {
            if (!game.force_one_square()) {
                break;
            }
            if (fp)
                game.print_json(fp, "\"type\":\"add dot\",");
        } else {
            if (fp)
                game.print_json(fp, "");
        }
        if (game.impossible()) {
            break;
        }
        // game.print_puzzle(false);
    }

    return game;
}

struct SolutionMetaData {
    int depth = 0;
    int max_width = 0;

    void print(const string& prefix, const string& suffix) {
        printf("%s", prefix.c_str());

        printf("\"depth\": %d, ", depth);
        printf("\"max_width\": %d", max_width);

        printf("%s", suffix.c_str());
    }
};

struct Classification {
    SolutionMetaData all;
    SolutionMetaData one_of;
    SolutionMetaData dep;
    SolutionMetaData square;
    SolutionMetaData cant_fit;
    SolutionMetaData cover;
    SolutionMetaData single_solution;
    SolutionMetaData uncontested_no_cover;

    bool solved = false;

    void print(const string& prefix, const string& suffix) {
        printf("%s", prefix.c_str());

        all.print("\"all\": {", "}, ");
        one_of.print("\"one_of\": {", "}, ");
        dep.print("\"dep\": {", "}, ");
        square.print("\"square\": {", "}, ");
        cant_fit.print("\"cant_fit\": {", "}, ");
        cover.print("\"cover\": {", "},");
        single_solution.print("\"single-solution\": {", "},");
        uncontested_no_cover.print("\"uncontested-no-cover\": {", "}");
        printf("%s", suffix.c_str());
    }
};

Classification classify_game(Game game,
                             FILE* print_progress=NULL) {
    Classification ret;

    game.reset_hints();
    const char* extra_json = "";

    for (int i = 0; ; ++i) {
        if (print_progress) {
            game.print_json(print_progress, extra_json);
        }

        auto res = game.iterate();
        switch (res.first) {
        case DeductionKind::NONE:
            return ret;
        case DeductionKind::COVER:
            extra_json = "\"type\":\"cover\",";
            ret.cover.depth++;
            ret.cover.max_width = std::max(ret.cover.max_width,
                                           res.second);
            break;
        case DeductionKind::CANT_FIT:
            extra_json = "\"type\":\"fit\",";
            ret.cant_fit.depth++;
            ret.cant_fit.max_width = std::max(ret.cant_fit.max_width,
                                              res.second);
            break;
        case DeductionKind::SQUARE:
            extra_json = "\"type\":\"square\",";
            ret.square.depth++;
            ret.square.max_width = std::max(ret.square.max_width,
                                            res.second);
            break;
        case DeductionKind::DEPENDENCY:
            extra_json = "\"type\":\"dep\",";
            ret.dep.depth++;
            ret.dep.max_width = std::max(ret.dep.max_width,
                                         res.second);
            break;
        case DeductionKind::ONE_OF:
            extra_json = "\"type\":\"oneof\",";
            ret.one_of.depth++;
            ret.one_of.max_width = std::max(ret.one_of.max_width,
                                           res.second);
            break;
        case DeductionKind::SINGLE_SOLUTION:
            extra_json = "\"type\":\"single-solution\",";
            ret.single_solution.depth++;
            ret.single_solution.max_width =
                std::max(ret.single_solution.max_width,
                         res.second);
            break;
        case DeductionKind::UNCONTESTED_NO_COVER:
            extra_json = "\"type\":\"uncontested-no-cover\",";
            ret.uncontested_no_cover.depth++;
            ret.uncontested_no_cover.max_width =
                std::max(ret.uncontested_no_cover.max_width,
                         res.second);
            break;
        }
        ret.all.depth++;
        ret.all.max_width = std::max(ret.all.max_width,
                                     res.second);

        if (game.solved()) {
            if (print_progress) {
                game.print_json(print_progress, extra_json);
            }
            ret.solved = true;
            break;
        }

        if (game.impossible()) {
            break;
        }
    }

    return ret;
}

Game mutate(Game game) {
    // game.reset_fixed();
    game.reset_forced();
    game.mutate();

    return game;
}

struct OptimizationResult {
    OptimizationResult(Game game, Classification cls)
        : game(game), cls(cls) {
        score =
            (cls.cover.depth * FLAGS_score_cover) +
            (cls.cant_fit.depth * FLAGS_score_cant_fit) +
            (cls.square.depth * FLAGS_score_square) +
            (cls.dep.depth * FLAGS_score_dep) +
            (cls.one_of.depth * FLAGS_score_one_of) +
            (cls.single_solution.depth * FLAGS_score_single_solution) +
            (cls.uncontested_no_cover.depth * FLAGS_score_uncontested_no_cover) +
            (cls.all.max_width * FLAGS_score_max_width);
    }

    Game game;
    Classification cls;
    int score;
};

Game minimize_width(Game game) {
    const int N = FLAGS_optimize_iterations;
    if (!N)
        return game;

    struct Cmp {
        bool operator() (const OptimizationResult& a,
                         const OptimizationResult& b) {
            return b.score < a.score;
        }
    };

    FILE* fp = nullptr;
    if (!FLAGS_solve_progress_file.empty()) {
        fp = fopen(FLAGS_solve_progress_file.c_str(), "w");
    }

    std::vector<OptimizationResult> res;
    res.emplace_back(game, classify_game(game));

    char buf[256];
    auto extra_json = [&] (int iter, int score) {
        sprintf(buf, "\"score\":%d,\"iter\":%d,", score, iter);
        return buf;
    };
    if (fp) {
        game.print_json(fp, extra_json(0, res[0].score));
    }

    for (int i = 0; i < N; ++i) {
        auto base = res[rand() % res.size()];
        Game opt = mutate(base.game);
        opt = add_forced_squares(opt, NULL);
        OptimizationResult opt_res(opt, classify_game(opt));

        if (opt_res.cls.solved) {
            // if (opt_res.score > res[0].score) {
            //     fprintf(stderr, "%d: %d\n", i, res[0].score);
            // }
            res.push_back(opt_res);
            if (fp &&
                opt_res.score > res[0].score) {
                opt.print_json(fp, extra_json(i, res[0].score));
            }
            sort(res.begin(), res.end(), Cmp());
            if (res.size() > 10) {
                res.pop_back();
            }
        }
    }
    // fprintf(stderr, "---\n");

    return res[0].game;
}

Game optimize_game(Game game) {
    Classification cls = classify_game(game);

    Game opt = minimize_width(game);
    Classification opt_cls = classify_game(opt);

    fprintf(stderr,
            "%d/%d [%d/%d/%d/%d/%d/%d/%d] -> %d/%d "
            "[%d/%d/%d/%d/%d/%d/%d]\n",
            cls.all.max_width,
            cls.all.depth,
            cls.one_of.depth,
            cls.dep.depth,
            cls.square.depth,
            cls.cant_fit.depth,
            cls.cover.depth,
            cls.single_solution.depth,
            cls.uncontested_no_cover.depth,
            opt_cls.all.max_width,
            opt_cls.all.depth,
            opt_cls.one_of.depth,
            opt_cls.dep.depth,
            opt_cls.square.depth,
            opt_cls.cant_fit.depth,
            opt_cls.cover.depth,
            opt_cls.single_solution.depth,
            opt_cls.uncontested_no_cover.depth);

    return opt;
}


// Find a game with the given parameters that can be solved.
Game create_candidate_game() {

    for (int i = 0; i < 1000000; ++i) {
        FILE* fp = nullptr;
        if (!FLAGS_candidate_progress_file.empty()) {
            fp = fopen(FLAGS_candidate_progress_file.c_str(), "w");
        }

        Game game;
        game = add_forced_squares(game, fp);
        if (fp)
            fclose(fp);

        if (game.solved()) {
            Classification cls = classify_game(game);
            if (!cls.solved) {
                continue;
            }
            if ((FLAGS_score_square > 0 || FLAGS_score_dep > 0 ||
                 FLAGS_score_one_of > 0) &&
                (cls.square.depth == 0 &&
                 cls.one_of.depth == 0 &&
                 cls.dep.depth == 0)) {
                continue;
            }
            return game;
        }
    }

    assert(false);
}

int solve(const std::string& puzzle) {
    Game game(puzzle);
    FILE* fp = NULL;
    if (!FLAGS_solve_progress_file.empty()) {
        fp = fopen(FLAGS_solve_progress_file.c_str(), "w");
    }
    Classification cls = classify_game(game, fp);

    printf("{ \"puzzle\": [");
    game.print_puzzle(true);
    cls.print("], \"classification\": {", "}");
    printf("}\n");

    return 0;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    srand(FLAGS_seed);

    if (!FLAGS_solve.empty()) {
        return solve(FLAGS_solve);
    }

    for (int j = 0; j < FLAGS_puzzle_count; ++j) {
        Game game = create_candidate_game();
        Game opt = optimize_game(game);
        Classification cls = classify_game(opt);

        printf("{ \"puzzle\": [");
        opt.print_puzzle(true);
        cls.print("], \"classification\": {", "}");
        printf("}\n");
    }
}
