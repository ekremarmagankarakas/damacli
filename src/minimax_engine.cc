#include "minimax_engine.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "board.h"
#include "piece_kind.h"

namespace {

constexpr int kInf = 1'000'000;
constexpr int kMateScore = 100'000;
constexpr int kManValue = 100;
constexpr int kKingValue = 300;

}  // namespace

MinimaxEngine::MinimaxEngine(int depth) : depth_(depth) {}

int MinimaxEngine::Evaluate(const Board& board) const {
  // Score from White's perspective: positive = White advantage.
  int score = 0;
  for (int row = 0; row < Board::kSize; ++row) {
    for (int col = 0; col < Board::kSize; ++col) {
      auto pk = board.At(row, col);
      if (!pk) continue;
      switch (*pk) {
        case PieceKind::WMan:  score += kManValue; break;
        case PieceKind::WKing: score += kKingValue; break;
        case PieceKind::BMan:  score -= kManValue; break;
        case PieceKind::BKing: score -= kKingValue; break;
        case PieceKind::Empty: break;
      }
    }
  }
  return score;
}

int MinimaxEngine::Search(Board& board, int depth, int alpha, int beta) {
  if (depth == 0) {
    return Evaluate(board);
  }
  auto moves = board.LegalMoves();
  if (moves.empty()) {
    // Side to move has no moves: loses. Prefer shorter losses for opponent.
    int sign = (board.SideToMove() == Color::kWhite) ? -1 : 1;
    return sign * (kMateScore - depth);
  }

  bool maximizing = board.SideToMove() == Color::kWhite;
  if (maximizing) {
    int best = -kInf;
    for (const Move& m : moves) {
      board.Apply(m);
      int score = Search(board, depth - 1, alpha, beta);
      board.Undo();
      best = std::max(best, score);
      alpha = std::max(alpha, best);
      if (beta <= alpha) {
        break;
      }
    }
    return best;
  } else {
    int best = kInf;
    for (const Move& m : moves) {
      board.Apply(m);
      int score = Search(board, depth - 1, alpha, beta);
      board.Undo();
      best = std::min(best, score);
      beta = std::min(beta, best);
      if (beta <= alpha) {
        break;
      }
    }
    return best;
  }
}

Move MinimaxEngine::Choose(Board& board) {
  auto moves = board.LegalMoves();
  if (moves.empty()) {
    throw std::runtime_error("MinimaxEngine: no legal moves available");
  }

  bool maximizing = board.SideToMove() == Color::kWhite;
  Move best_move = moves[0];
  int best_score = maximizing ? -kInf : kInf;
  int alpha = -kInf;
  int beta = kInf;

  for (const Move& m : moves) {
    board.Apply(m);
    int score = Search(board, depth_ - 1, alpha, beta);
    board.Undo();
    if (maximizing) {
      if (score > best_score) {
        best_score = score;
        best_move = m;
      }
      alpha = std::max(alpha, best_score);
    } else {
      if (score < best_score) {
        best_score = score;
        best_move = m;
      }
      beta = std::min(beta, best_score);
    }
  }
  return best_move;
}
