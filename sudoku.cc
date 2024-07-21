#include <vector>
#include <array>
#include <functional>
#include <bitset>
#include <iostream>

using namespace std;

// Updates the possible values for the possibilities board, given that the
// value in `value` is placed at position (i, j)
void Update(array<array<bitset<9>, 9>, 9>& possibilities, int i, int j, char value) {
	int box_start_row = i / 3 * 3;
	int box_start_col = j / 3 * 3;
	for (int k = 0; k < 9; k++) {
		possibilities[i][k].reset(value);
		possibilities[k][j].reset(value);
		possibilities[box_start_row + k / 3][box_start_col + k % 3].reset(
				value);
	}
	possibilities[i][j] = bitset<9>();
}

// Finds if there is only one square in the box, column, or row (whose tiles are given
// by putting the values 0-8 into locator) that can have a given value, if so put that
// value in that tile, and update possibilities, board, and unfilled.
void FindForcing(array<array<bitset<9>, 9>, 9>& possibilities, vector<vector<char>>& board,
		function<tuple<int, int>(int)> locator, int& unfilled) {
	array<bitset<9>, 9> num_locs;
	for (int i = 0; i < 9; i++) {
		int row, col;
		tie(row, col) = locator(i);
		for (int b = 0; b < 9; b++) {
			if (possibilities[row][col][b]) {
				num_locs[b].set(i);
			}
		}
	}
	for (int i = 0; i < 9; i++) {
		if (num_locs[i].count() == 1) {
			int set;
			for (set = 0; !num_locs[i][set]; set++)
				;
			int row, col;
			tie(row, col) = locator(set);
			Update(possibilities, row, col, i);
			unfilled--;
			board[row][col] = i + '1';
		}
	}
}

// Finds if there are n squares in the box, column, or row (whose tiles are given
// by putting the values 0-8 into locator) that must hold the same n values. (e.g.
// if there are 2 squares which could either be a 2 or a 4) then ensure that the
// possibilities table marks those values as impossible in any other tile in the
// box, column, or row.
void FindSaturated(array<array<bitset<9>, 9>, 9>& possibilities,
		function<tuple<int, int>(int)> locator) {
	unordered_map<bitset<9>, bitset<9>> counts;
	for (int i = 0; i < 9; i++) {
		int row, col;
		tie(row, col) = locator(i);
		bitset<9> poss = possibilities[row][col];
		counts.emplace(poss, bitset<9>());
		counts[poss].set(i);
	}
	for (auto& [key, value] : counts) {
		if (key.count() == value.count()) {
			for (int i = 0; i < 9; i++) {
				if (!value[i]) {
					int row, col;
					tie(row, col) = locator(i);
					possibilities[row][col] &= ~key;
				}
			}
		}
	}
}

struct SolveState {
	array<array<bitset<9>, 9>, 9>possibilities;
	vector<vector<char>> board;
	int num_unfilled;
	int guess_row;
	int guess_col;
	int guessed;
};


void solveSudoku(vector<vector<char>>& board) {
	int unfilled = 81;
	int guessed = 0;
	array<array<bitset<9>, 9>, 9>possibilities;
	std::vector<SolveState> backtrack_stack;
	for (int i = 0; i < 9; i++)
		possibilities[i].fill(bitset<9>(0x1ff));

	// Fill in initial board state.
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			if (board[i][j] != '.') {
				unfilled--;
				Update(possibilities, i, j, board[i][j] - '1');
			}
		}
	}

	int prev_unfilled = unfilled;
	while (unfilled > 0) {
		// Fill in cells with only one possibility.
		for (int i = 0; i < 9; i++) {
			for (int j = 0; j < 9; j++) {
				if (possibilities[i][j].count() == 1) {
					int set;
					for (set = 0; !possibilities[i][j][set]; set++)
						;
					unfilled--;
					Update(possibilities, i, j, set);
					board[i][j] = set + '1';
				}
			}
		}

		// Process rows with forced and saturated values.
		for (int i = 0; i < 9; i++) {
			auto locator = [i](int j) { return make_tuple(i, j); };
			FindForcing(possibilities, board, locator, unfilled);
			FindSaturated(possibilities, locator);
		}
		// Process columns with forced and saturated values.
		for (int i = 0; i < 9; i++) {
			auto locator = [i](int j) { return make_tuple(j, i); };
			FindForcing(possibilities, board, locator, unfilled);
			FindSaturated(possibilities, locator);
		}
		// Process boxes with forced and saturated values.
		for (int i = 0; i < 9; i++) {
			int box_row = i / 3 * 3;
			int box_col = i % 3 * 3;
			auto locator = [&](int j) {
				return make_tuple(box_row + j / 3, box_col + j % 3);
			};
			FindForcing(possibilities, board, locator, unfilled);
			FindSaturated(possibilities, locator);
		}

		// If we go a full round of heuristic rules without making progress, guess.
		if (prev_unfilled == unfilled) {
			int row = -1, col = -1;
			int min = 10;
			// Find the square with the fewest options
			for (int i = 0; i < 9; i++) {
				for (int j = 0; j < 9; j++) {
					if (board[i][j] == '.' &&
							possibilities[i][j].count() < min) {
						min = possibilities[i][j].count();
						row = i;
						col = j;
					}
				}
			}

			// Guess, backtracking when necessary.
			while (true) {
				for (; guessed < 9; guessed++) {
					if (possibilities[row][col][guessed]) {
						break;
					}
				}

				if (guessed < 9) {
					// We have a guess, store information we need to backtrack.
					backtrack_stack.push_back({
							possibilities, board, unfilled, row, col, guessed});
					Update(possibilities, row, col, guessed);
					board[row][col] = guessed + '1';
					guessed = 0;
					unfilled--;
					break;
				} else {
					// We have no valid guess, backtrack.
					if(backtrack_stack.empty()) {
						cout << "Invalid sudoku board input, it has no solution." << endl;
						return;
					}
					SolveState s = std::move(backtrack_stack.back());
					possibilities = std::move(s.possibilities);
					board = std::move(s.board);
					unfilled = std::move(s.num_unfilled);
					row = std::move(s.guess_row);
					col = std::move(s.guess_col);
					guessed = std::move(s.guessed);
					backtrack_stack.pop_back();
					guessed++;
				}
			}
		} else {
			prev_unfilled = unfilled;
		}
	}
}

/*
.....7..9
.4..812..
...9...1.
..53...72
293....5.
.....53..
8...23...
7...5..4.
531.7....
*/

int main() {
    cout << "Enter your sudoku board in 9 rows of 9 characters each," << endl;
    cout << "each character may be 1 thru 9, or '.' if it's unknown:" << endl;
	vector<vector<char>> board; 
	for(int i = 0; i < 9; i++) {
		string s;
		cin >> s;
		if(s.size() != 9) {
			cout << "Each row in a sudoku board must have 9 numbers in it!" << endl;
			return -1;
		}
		for(char c: s) {
			if((c < '1' || c > '9') && c != '.') {
				cout << "The valid characters for the board are 1 through 9 and . to specify unknown" << endl;
				return -1;
			}
		}
		board.push_back(vector<char>(s.begin(), s.end()));
	}

	solveSudoku(board);
	for(int i = 0; i < 9; i++) {
		for(int j = 0; j < 9; j++) {
			cout << board[i][j] << ' ';
		}
		cout << endl;
	}
}
