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

DEFINE_bool(disallow_basic, false,
            "If set, never generate puzzles that require only "
            "'basic' deduction.");

class Game {
public:
    static const int W = MAP_WIDTH + 1, H = MAP_HEIGHT, N = PIECES;

    using Hint = std::pair<uint16_t, uint16_t>;
    using Mask = uint64_t;
    using MaskArray = std::array<Mask, H * W>;
    using DepMask = std::bitset<H * W>;

    struct Options {
        bool dep_ = true;
        bool dep_non_forced_ = true;
        bool xxx_ = true;
    };

    Game() : Game(Options()) {
    }

    Game(Options opt) : opt_(opt) {
        for (int r = 0; r < H; ++r) {
            border_[r * W] = 1;
        }
        randomize();
        reset_hints();
        reset_possible();
        orig_possible_ = possible_;
    }

    void randomize() {
        for (int i = 0; i < N; ++i) {
            while (1) {
                int at = random() % (W * H);
                int val = 2 + random() % 4;
                if (!fixed_[at] && !border_[at]) {
                    hints_.emplace_back(at, val);
                    fixed_[at] = piece_mask(i);
                    break;
                }
            }
        }
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
        for (auto& dep : dependent_) {
            dep.reset();
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
                                       valid_orientation_[p]);
                                // orientation_count(p));
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

    void reset_possible() {
        for (int at = 0; at < W * H; ++at) {
            possible_[at] = 0;
        }
        for (int piece = 0; piece < N; ++piece) {
            update_possible(piece);
        }
        if (opt_.dep_) {
            update_dependent();
        }
        if (opt_.xxx_) {
            update_yyy();
        }
    }

    int reset_fixed() {
        int count = 0;
        for (int piece = 0; piece < N; ++piece) {
            count += update_fixed(piece);
        }
        return count;
    }

    void reset_forced() {
        for (int i = 0; i < W * H; ++i) {
            forced_[i] = false;
        }
    }

    bool force_one_square() {
        int best_at = -1;
        int best_score = -1;
        int best_score_count;

        for (int at = 0; at < W *H; ++at) {
            if (fixed_[at] || forced_[at] || !possible_[at]) {
                continue;
            }
            int score = orig_possible_count(at) - possible_count(at);
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

    bool impossible() {
        for (int piece = 0; piece < N; ++piece) {
            if (!valid_orientation_[piece])
                return true;
        }

        return false;
    }

    bool solved() {
        for (int piece = 0; piece < N; ++piece) {
            if (orientation_count(piece) != 1) {
                return false;
            }
        }

        return true;
    }

    void mutate() {
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
        default:
            break;
        }

        reset_hints();
        reset_possible();
        orig_possible_ = possible_;
    }

    Options opt_;

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

    void update_possible(int piece) {
        Mask mask = piece_mask(piece);
        int at = hints_[piece].first;
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                int offset = (size - 1) - (o >> 1);
                int step = ((o & 1) ? W : 1);
                bool ok = do_squares(size, at + offset * -step, step,
                                     [&] (int at) {
                                         return (!fixed_[at] ||
                                                 fixed_[at] == mask);
                                     });
                if (ok) {
                    do_squares(size, at + offset * -step, step,
                               [&] (int at) {
                                   possible_[at] |= mask;
                                   return true;
                               });
                }
            }
        }
    }

    int update_fixed(int piece) {
        Mask mask = piece_mask(piece);
        int piece_at = hints_[piece].first;
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        int valid_count = 0;
        std::array<uint8_t, W*H> count = { 0 };

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                int offset = (size - 1) - (o >> 1);
                int step = ((o & 1) ? W : 1);
                if (do_squares(size, piece_at + offset * -step, step,
                               [&] (int at) {
                                   return possible_[at] & mask;
                               })) {
                    ++valid_count;
                    do_squares(size, piece_at + offset * -step, step,
                               [&] (int at) {
                                   count[at]++;
                                   return true;
                               });
                }
            }
        }

        int updated = 0;
        for (int at = 0; at < W * H; ++at) {
            if (!fixed_[at]) {
                if ((count[at] == valid_count) ||
                    (forced_[at] && possible_[at] == mask)) {
                    updated = 1;
                    fixed_[at] = mask;
                    update_not_possible(at, mask, piece);
                }
            }
        }

        return updated;
    }

    void update_not_possible(int update_at, Mask mask, int piece) {
        int at = hints_[piece].first;
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                int offset = (size - 1) - (o >> 1);
                int step = ((o & 1) ? W : 1);
                bool no_intersect =
                    do_squares(size, at + offset * -step, step,
                               [&] (int at) {
                                   return at != update_at;
                               });
                if (no_intersect) {
                    valid_orientation_[piece] &= ~(1 << o);
                }
            }
        }
    }

    int init_valid_orientations(int piece) {
        int at = hints_[piece].first;
        int size = hints_[piece].second;
        int ret = 0;
        for (int o = 0; o < size * 2; ++o) {
            int offset = (size - 1) - (o >> 1);
            int step = ((o & 1) ? W : 1);
            bool fit =
                do_squares(size, at + offset * -step, step,
                           [&] (int test_at) {
                               if (test_at != at && fixed_[test_at]) {
                                   return false;
                               }
                               return true;
                           });
            if (fit) {
                ret |= (1 << o);
            }
        }
        return ret;
    }

    bool do_squares(int n, int start, int step,
                    std::function<bool(int)> fun) {
        for (int i = 0; i < n; ++i) {
            int at = start + i * step;
            if (at < 0 || at >= W * H || border_[at]) {
                return false;
            }

            if (!(fun(at))) {
                return false;
            }
        }

        return true;
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
                        if (opt_.dep_non_forced_) {
                            possible_[target] &= possible_[at];
                        } else {
                            if (forced_[target]) {
                                possible_[target] = possible_[at] =
                                    (possible_[target] & possible_[at]);
                            }
                        }
                    }
                }
            }
        }
    }

    DepMask find_dependent(int piece, int target) {
        Mask mask = piece_mask(piece);
        int at = hints_[piece].first;
        int size = hints_[piece].second;
        int valid_o = valid_orientation_[piece];
        bool ret_valid;
        DepMask ret;
        ret.set();

        for (int o = 0; o < size * 2; ++o) {
            if (valid_o & (1 << o)) {
                int offset = (size - 1) - (o >> 1);
                int step = ((o & 1) ? W : 1);
                bool overlaps_target = false;
                bool ok = do_squares(size, at + offset * -step, step,
                                     [&] (int at) {
                                         if (at == target)
                                             overlaps_target = true;
                                         return (!fixed_[at] ||
                                                 fixed_[at] == mask);
                                     });
                DepMask mask;
                if (ok && overlaps_target) {
                    do_squares(size, at + offset * -step, step,
                               [&] (int at) {
                                   mask[at] = true;
                                   return true;
                               });
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
    void update_yyy() {
        for (int piece = 0; piece < N; ++piece) {
            int size = hints_[piece].second;
            int at = hints_[piece].first;
            std::vector<int> covered;
            for (int at = 0; at < W * H; ++at) {
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
                            int offset = (size - 1) - (o >> 1);
                            int step = ((o & 1) ? W : 1);
                            bool no_candidate =
                                do_squares(size, at + offset * -step, step,
                                           [&] (int at) {
                                               return at != ai && at != aj;
                                           });
                            if (no_candidate) {
                                valid_orientation_[piece] &= ~(1 << o);
                            }
                        }
                    }
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

    std::vector<Hint> hints_;
    uint16_t valid_orientation_[N] { 0 };
    MaskArray orig_possible_;
    MaskArray possible_ = { 0 };
    MaskArray fixed_ = { 0 };
    std::array<DepMask, H * W> dependent_;
    bool forced_[H * W] = { 0 };
    bool border_[H * W] = { 0 };
};

Game add_forced_squares(Game game) {
    while (1) {
        game.reset_possible();

        if (!game.reset_fixed()) {
            if (!game.force_one_square()) {
                break;
            }
        }
        if (game.impossible()) {
            break;
        }
    }

    return game;
}

struct SolutionMetaData {
    bool solved = false;
    int depth = 0;
    int max_width = 0;

    void print(const string& prefix, const string& suffix) {
        printf("%s", prefix.c_str());

        printf("\"solved\": %d, ", solved);
        printf("\"depth\": %d, ", depth);
        printf("\"max_width\": %d", max_width);

        printf("%s", suffix.c_str());
    }
};

SolutionMetaData solve_game_with_options(Game game,
                                         Game::Options opt) {
    SolutionMetaData ret;

    game.reset_hints();
    game.opt_ = opt;

    for (int i = 0; ; ++i) {
        game.reset_possible();

        int width = game.reset_fixed();
        if (!width) {
            break;
        }
        ret.max_width = std::max(ret.max_width, width);

        if (game.solved()) {
            ret.solved = true;
            ret.depth = i;
            break;
        }

        if (game.impossible()) {
            break;
        }
    }

    return ret;
}

struct Classification {
    SolutionMetaData all;
    SolutionMetaData no_dep;
    SolutionMetaData no_xxx;
    SolutionMetaData basic;

    void print(const string& prefix, const string& suffix) {
        printf("%s", prefix.c_str());

        all.print("\"all\": {", "}, ");
        no_dep.print("\"no_dep\": {", "}, ");
        no_xxx.print("\"no_xxx\": {", "}, ");
        basic.print("\"basic\": {", "}");
        printf("%s", suffix.c_str());
    }
};

Classification classify_game(Game game) {
    Classification ret;

    Game::Options all;
    ret.all = solve_game_with_options(game, all);

    Game::Options no_dep;
    no_dep.dep_ = false;
    ret.no_dep = solve_game_with_options(game, no_dep);

    Game::Options no_xxx;
    no_xxx.xxx_ = false;
    ret.no_xxx = solve_game_with_options(game, no_xxx);

    Game::Options basic;
    basic.dep_ = basic.dep_non_forced_ = basic.xxx_ = false;
    ret.basic = solve_game_with_options(game, basic);

    return ret;
}

Game mutate(Game game) {
    game.reset_fixed();
    game.reset_forced();
    game.mutate();

    return game;
}

struct OptimizationResult {
    OptimizationResult(Game game, Classification cls)
        : game(game), cls(cls) {
    }

    Game game;
    Classification cls;
};

Game minimize_width(Game game) {
    struct Cmp {
        bool operator() (const OptimizationResult& a,
                         const OptimizationResult& b) {
            return a.cls.all.max_width < b.cls.all.max_width;
        }
    };

    std::vector<OptimizationResult> res;
    res.emplace_back(game, classify_game(game));

    for (int i = 0; i < 10000; ++i) {
        auto base = res[rand() % res.size()];
        Game opt = mutate(base.game);
        opt = add_forced_squares(opt);
        Classification opt_cls = classify_game(opt);

        if (FLAGS_disallow_basic && opt_cls.basic.solved) {
            continue;
        }
        if (opt_cls.all.solved &&
            opt_cls.all.max_width < base.cls.all.max_width) {
            res.emplace_back(opt, opt_cls);
            sort(res.begin(), res.end(), Cmp());
            if (res.size() > 10) {
                res.pop_back();
            }
        }
    }

    return res[0].game;
}

Game optimize_game(Game game) {
    Classification cls = classify_game(game);

    Game opt = minimize_width(game);
    Classification opt_cls = classify_game(opt);

    fprintf(stderr, "%d/%d -> %d/%d\n",
            cls.all.max_width,
            cls.all.depth,
            opt_cls.all.max_width,
            opt_cls.all.depth);

    return opt;
}


// Find a game with the given parameters that can be solved.
Game create_candidate_game() {
    for (int i = 0; i < 100000; ++i) {
        Game game;
        game = add_forced_squares(game);

        if (game.solved()) {
            Classification cls = classify_game(game);
            if (!cls.all.solved) {
                fprintf(stderr, "wtf?\n");
                continue;
            }
            if (FLAGS_disallow_basic && cls.basic.solved) {
                continue;
            }
            return game;
        }
    }

    assert(false);
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    srand(FLAGS_seed);

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
