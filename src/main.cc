#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <vector>

class Game {
    static const int W = 13, H = 12, N = 33;

public:
    using Hint = std::pair<uint16_t, uint16_t>;
    using Mask = uint64_t;
    using MaskArray = std::array<Mask, H * W>;
    using DepMask = std::bitset<H * W>;

    struct Options {
        bool dep_ = true;
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

    void print_puzzle() {
        std::map<int, int> hints;
        for (auto hint : hints_) {
            hints[hint.first] = hint.second;
        }

        int at = 0;
        for (int r = 0; r < H; ++r) {
            printf("\"");
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
            printf("\",\n");
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
    }

    bool reset_fixed() {
        bool updated = false;
        for (int piece = 0; piece < N; ++piece) {
            updated |= update_fixed(piece);
        }
        return updated;
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

    bool update_fixed(int piece) {
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

        bool updated = false;
        for (int at = 0; at < W * H; ++at) {
            if (!fixed_[at]) {
                if ((count[at] == valid_count) ||
                    (forced_[at] && possible_[at] == mask)) {
                    updated = true;
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

    void update_dependent() {
        for (int at = 0; at < W * H; ++at) {
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
                        possible_[at] = possible_[target] =
                            (possible_[at] & possible_[target]);
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
        DepMask ret;

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
                if (ok && overlaps_target) {
                    do_squares(size, at + offset * -step, step,
                               [&] (int at) {
                                   ret[at] = true;
                                   return true;
                               });
                }
            }
        }

        return ret;
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

int main(int argc, char** argv) {
    srandom(atoi(argv[1]));
    for (int j = 0; j < 1000000; ++j) {
        Game game;
        for (int i = 0; i < 100; ++i) {
            game.reset_possible();
            // game.print_possible();
            // printf("\n-----------------------------------------------\n\n");
            if (!game.reset_fixed()) {
                if (!game.force_one_square()) {
                    break;
                }
            }
        }

        if (game.impossible()) {
            continue;
        }

        if (!game.solved()) {
            continue;
        }

        int solve_dep = 0, solve_nodep = 0;
        game.reset_hints();
        for (int i = 0; i < 100; ++i) {
            game.reset_possible();
            if (!game.reset_fixed()) {
                // if (i >= 12) {
                //     printf("dep rounds: %d\n", i);
                //     game.print_puzzle();
                // }
                solve_dep = i;
                break;
            }
        }
        game.reset_hints();
        game.opt_.dep_ = false;
        for (int i = 0; i < 100; ++i) {
            game.reset_possible();
            if (!game.reset_fixed()) {
                // if (i >= 12) {
                //     printf("nodep rounds: %d [%d]\n", i, solve_dep);
                //     game.print_puzzle();
                // }
                solve_nodep = i;
                break;
            }
        }

        if (solve_dep && !solve_nodep) {
            printf("DEP=%d NODEP=%d\n", solve_dep, solve_nodep);
            game.print_puzzle();

            // game.reset_hints();
            // game.opt_.dep_ = true;
            // for (int i = 0; i < 100; ++i) {
            //     game.reset_possible();
            //     game.print_possible();
            //     printf("\n-----------------------------------------------\n\n");
            //     if (!game.reset_fixed()) {
            //         // if (i >= 12) {
            //         //     printf("nodep rounds: %d\n", i);
            //         //     game.print_puzzle();
            //         // }
            //         break;
            //     }
            // }
        }
    }
}
