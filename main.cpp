#include <raylib.h>
#include <array>
#include <vector>
#include <initializer_list>
#include <iostream>
#include <algorithm>
#include <random>
#include <cassert>
using std::array;
using std::vector;
using ssize_t = std::ptrdiff_t;

template <typename T, ssize_t x, ssize_t y> using mdarray = array<array<T, x>, y>;
template <typename T> using mdvector = vector<vector<T /* x */> /* y */>;
using block = int;
enum class rotate_to { NONE = 0, LEFT, RIGHT };

struct tetromino
{
    ssize_t x, y, size;
    tetromino() {}
    tetromino(ssize_t x, ssize_t y) : x(x), y(y), size(0) {}
    tetromino& operator = (const mdvector<block>& _list)
    {
        internal_vector = _list;
        if (!std::all_of(internal_vector.begin(), internal_vector.end(), [&] (decltype(internal_vector)::value_type& b) { return b.size() == internal_vector.size(); }))
        {
            std::cerr
                << "A piece's width and height must be the same!\n"
                << "(Piece vector was " << internal_vector.back().size() << 'x' << internal_vector.size() << ")\n";
        }
        size = internal_vector.size();
        return *this;
    }
    tetromino(ssize_t x, ssize_t y, const mdvector<block>& list) : tetromino(x, y) { *this = list; }
          block& at(ssize_t x, ssize_t y)       { return internal_vector[y][x]; }
    const block& at(ssize_t x, ssize_t y) const { return internal_vector[y][x]; }
    tetromino& rotate(rotate_to direction)
    {
        mdvector<block> piece_copy(internal_vector);
        auto transpose = [&]
        {
            for (ssize_t y = 0; y < size; y++)
            {
                for (ssize_t x = 0; x < size; x++)
                {
                    std::swap(internal_vector.at(x).at(y), piece_copy.at(y).at(x));
                }
            }
        };
        auto reverse_cols = [&] { std::reverse(internal_vector.begin(), internal_vector.end()); };
        auto reverse_rows = [&] { for (auto& row : internal_vector) std::reverse(row.begin(), row.end()); };
        switch (direction)
        {
            case rotate_to::LEFT:
                transpose();
                reverse_cols();
                break;
            case rotate_to::RIGHT:
                transpose();
                reverse_rows();
                break;
            default: break;
        }
        return *this;
    }
private:
    mdvector<block> internal_vector;
} current_piece(5, 0), hold(5, 0);

struct grid
{
    ssize_t width, height;
    ssize_t block_size = 15;
    grid() noexcept(true)
    {
        for (auto& row : internal_array) row.fill(0);
        width = internal_array.back().size();
        height = internal_array.size();
    }
    block& at(ssize_t x, ssize_t y) { return internal_array[y][x]; }
    bool collides(const tetromino& piece, ssize_t offsetx = 0, ssize_t offsety = 0)
    {
        for (ssize_t x = piece.x; x < piece.x + piece.size; x++)
        {
            for (ssize_t y = piece.y; y < piece.y + piece.size; y++)
            {
                if (piece.at(x - piece.x, y - piece.y) != 0 && this->at(x + offsetx, y) != 0) return true;
                if (piece.at(x - piece.x, y - piece.y) != 0 && this->at(x, y + offsety) != 0) return true;
            }
        }
        return false;
    }

    bool at_bottom(const tetromino& piece)
    {
        auto last_rows_empty = [&] () -> ssize_t
        {
            for (ssize_t y = piece.size - 1; y > 0; y--)
                for (ssize_t x = 0; x < piece.size; x++)
                    if (piece.at(x, y) != 0) return (piece.size - 1) - y;
            return 0;
        }();
        return (piece.y + piece.size - last_rows_empty >= this->height) || collides(piece, 0, 1);
    }
    auto on_walls(const tetromino& piece)
    {
        struct { bool left = false, right = false; } result;
        for (ssize_t y = 0; y < piece.size; y++)
            for (ssize_t x = piece.x; x < piece.x + piece.size; x++)
            {
                if (piece.at(x - piece.x, y) != 0)
                {
                    if (x <= 0) result.left = true;
                    if (x >= this->width - 1) result.right = true;
                }
            }
        return result;
    }

    void clear_lines()
    {
        for (auto& row : internal_array)
        {
            if (std::all_of(row.begin(), row.end(), [] (block& b) { return (b != 0); }))
            {
                row.fill(0);
                for (ssize_t index = &row - &internal_array.front(); index > 0; index--)
                {
                    internal_array[index] = internal_array[index - 1];
                }
                internal_array.begin()->fill(0);
            }
        }
    }
    void emplace(tetromino& piece)
    {
        for (ssize_t y = piece.y; y < piece.y + piece.size; y++)
            for (ssize_t x = piece.x; x < piece.x + piece.size; x++)
                if (piece.at(x - piece.x, y - piece.y) != 0) at(x, y) = piece.at(x - piece.x, y - piece.y);
    }
    void clear() { for (auto& row : internal_array) row.fill(0); }

private:
    mdarray<block, 10, 22> internal_array;
} grid;

array<Color, 8> palette =
{
    WHITE,
    BLUE,
    DARKBLUE,
    ORANGE,
    YELLOW,
    GREEN,
    PURPLE,
    RED
};

array<mdvector<block>, 7> pieces =
{
    mdvector<block>
    {
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0},
    },
    mdvector<block>
    {
        {0,0,2},
        {2,2,2},
        {0,0,0},
    },
    mdvector<block>
    {
        {3,0,0},
        {3,3,3},
        {0,0,0},
    },
    mdvector<block>
    {
        {4,4},
        {4,4},
    },
    mdvector<block>
    {
        {0,5,5},
        {5,5,0},
        {0,0,0},
    },
    mdvector<block>
    {
        {0,6,0},
        {6,6,6},
        {0,0,0},
    },
    mdvector<block>
    {
        {7,7,0},
        {0,7,7},
        {0,0,0},
    },
};

size_t bag_index = 0;
decltype(pieces) next_bag(pieces);
mdvector<block>& random_piece()
{
    if (bag_index >= 6)
    {
        bag_index = 0;
        std::swap(pieces, next_bag);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(next_bag.begin(), next_bag.end(), gen);
    }
    bag_index++;
    return pieces[bag_index - 1];
}

int main()
{
    InitWindow(600, 600, "tetris");

    int screen_center_x = GetScreenWidth() / 2;
    int screen_center_y = GetScreenHeight() / 2;

    ssize_t framescount = 0;
    int stepdelay = 750;
    bool hard_dropping = false;
    bool swapped = false;
    bool game_over = false;

    current_piece = random_piece();

    while (!WindowShouldClose())
    {
        if (framescount % stepdelay == 0 || hard_dropping)
        {
            if (game_over) { grid.clear(); game_over = false; }
            if (!grid.at_bottom(current_piece)) current_piece.y++;
            else
            {
                if (current_piece.y < 1) game_over = true;
                grid.emplace(current_piece);
                current_piece = tetromino(5, 0, random_piece());
                grid.clear_lines();
                hard_dropping = false;
                swapped = false;
                continue;
            }
            if (hard_dropping) continue;
        }
        if (IsKeyPressed(KEY_DOWN)) stepdelay = 100;
        else if (IsKeyReleased(KEY_DOWN)) stepdelay = 550;
        if (IsKeyPressed(KEY_SPACE)) hard_dropping = true;
        if (IsKeyPressed(KEY_LEFT) && !grid.on_walls(current_piece).left)
        {
            if (!grid.collides(current_piece, -1))
            {
                current_piece.x--;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) && !grid.on_walls(current_piece).right)
        {
            if (!grid.collides(current_piece, +1))
            {
                current_piece.x++;
            }
        }
        if (IsKeyPressed(KEY_Z))
        {
            if      (!grid.collides(tetromino(current_piece).rotate(rotate_to::LEFT)))      {                    current_piece.rotate(rotate_to::LEFT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::LEFT), +1))   { current_piece.x--; current_piece.rotate(rotate_to::LEFT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::LEFT), -1))   { current_piece.x++; current_piece.rotate(rotate_to::LEFT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::LEFT), 0, 1)) { current_piece.y--; current_piece.rotate(rotate_to::LEFT); }
        }
        if (IsKeyPressed(KEY_X))
        {
            if      (!grid.collides(tetromino(current_piece).rotate(rotate_to::RIGHT)))      {                    current_piece.rotate(rotate_to::RIGHT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::RIGHT), -1))   { current_piece.x--; current_piece.rotate(rotate_to::RIGHT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::RIGHT), +1))   { current_piece.x++; current_piece.rotate(rotate_to::RIGHT); }
            else if (grid.collides(tetromino(current_piece).rotate(rotate_to::RIGHT), 0, 1)) { current_piece.y--; current_piece.rotate(rotate_to::RIGHT); }
        }
        if (IsKeyPressed(KEY_C) && !swapped)
        {
            if (hold.size == 0)
            {
                hold = current_piece;
                current_piece = tetromino(5, 0, random_piece());
            }
            else
            {
                hold.x = 5; hold.y = 0;
                current_piece.x = 5; current_piece.y = 0;
                std::swap(hold, current_piece);
            }
            swapped = true;
        }

        BeginDrawing();
        ClearBackground(WHITE);

        for (ssize_t y = 0; y < grid.height; y++)
        {
            for (ssize_t x = 0; x < grid.width; x++)
            {
                int screen_y = (GetScreenHeight() / 2) + (y - grid.height / 2) * grid.block_size;
                int screen_x = (GetScreenWidth() / 2) + (x - grid.width / 2) * grid.block_size;
                DrawRectangleLines(screen_x, screen_y, grid.block_size, grid.block_size, Fade(BLACK, 1.0f));

                if (x >= current_piece.x && x < current_piece.x + current_piece.size)
                    if (y >= current_piece.y && y < current_piece.y + current_piece.size)
                    {
                        auto index = current_piece.at(x - current_piece.x, y - current_piece.y);
                        if (index != 0) DrawRectangle(screen_x, screen_y, grid.block_size, grid.block_size, palette.at(index));
                    }

                if (grid.at(x, y) != 0) DrawRectangle(screen_x, screen_y, grid.block_size, grid.block_size, palette.at(grid.at(x, y)));

            }
        }

        int holdposx = screen_center_x - screen_center_x / 2;
        int holdposy = screen_center_y - (grid.height * grid.block_size) / 2;
        for (ssize_t y = 0; y < hold.size; y++)
        {
            for (ssize_t x = 0; x < hold.size; x++)
            {
                if (hold.at(x, y) != 0)
                {
                    int hpx = holdposx + x * grid.block_size;
                    int hpy = holdposy + y * grid.block_size;

                    DrawRectangle(hpx + grid.block_size, hpy + grid.block_size, grid.block_size, grid.block_size, palette.at(hold.at(x, y)));
                }
            }
        }
        DrawRectangleLines(holdposx + grid.block_size, holdposy + grid.block_size, grid.block_size * hold.size, grid.block_size * hold.size, BLACK);

        int bagposx = screen_center_x + screen_center_x / 2;
        int bagposy = screen_center_y - (grid.height * grid.block_size) / 2;

        for (ssize_t index = bag_index; index < bag_index + 6; index++)
        {
            auto& piece = index < 6 ? pieces.at(index) : next_bag.at(index - 6);
            for (ssize_t x = 0; x < piece.size(); x++)
            {
                for (ssize_t y = 0; y < piece.size(); y++)
                {
                    ssize_t distance = index - (ssize_t)bag_index;

                    int bpx = bagposx + x * grid.block_size;
                    int bpy = bagposy + y * grid.block_size;
                    if (piece[y][x] != 0 && index >= 0)
                        DrawRectangle(bpx, bpy + distance * grid.block_size * 4, grid.block_size, grid.block_size, palette.at(piece[y][x]));
                }
            }
        }

        DrawRectangleLines(bagposx - grid.block_size, bagposy - grid.block_size, grid.block_size * 4 + grid.block_size, grid.block_size * 4 * 6 + grid.block_size, BLACK);

        EndDrawing();
        framescount++;
    }

    CloseWindow();
    return 0;
}

