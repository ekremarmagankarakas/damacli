#include "board.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

namespace {

constexpr int SqIdx(int row, int col) { return row * 8 + col; }
constexpr std::uint64_t Bit(int sq) { return 1ULL << sq; }
constexpr std::uint64_t BitRC(int row, int col) { return Bit(SqIdx(row, col)); }

constexpr int kDR[4] = {1, -1, 0, 0};   // N, S, E, W
constexpr int kDC[4] = {0, 0, 1, -1};

// Man movement directions: white moves N+E+W, black moves S+E+W.
constexpr std::array<int, 3> kWhiteManDirs = {0, 2, 3};
constexpr std::array<int, 3> kBlackManDirs = {1, 2, 3};

bool InBoardRC(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

// Compact mutable view of all four bitboards passed around generators.
struct Pos {
  std::uint64_t w_men;
  std::uint64_t b_men;
  std::uint64_t w_king;
  std::uint64_t b_king;
};

inline std::uint64_t AllOcc(const Pos& p) {
  return p.w_men | p.b_men | p.w_king | p.b_king;
}

inline bool IsEmpty(const Pos& p, int sq) {
  return !(AllOcc(p) & Bit(sq));
}

inline bool IsEnemy(const Pos& p, int sq, Color side) {
  std::uint64_t b = Bit(sq);
  return (side == Color::kWhite) ? ((p.b_men | p.b_king) & b)
                                 : ((p.w_men | p.w_king) & b);
}

inline bool IsOwn(const Pos& p, int sq, Color side) {
  std::uint64_t b = Bit(sq);
  return (side == Color::kWhite) ? ((p.w_men | p.w_king) & b)
                                 : ((p.b_men | p.b_king) & b);
}

enum class PieceBb { WMen, BMen, WKing, BKing };

PieceBb PieceAt(const Pos& p, int sq) {
  std::uint64_t b = Bit(sq);
  if (p.w_men & b) return PieceBb::WMen;
  if (p.b_men & b) return PieceBb::BMen;
  if (p.w_king & b) return PieceBb::WKing;
  return PieceBb::BKing;
}

void RemoveAt(Pos& p, int sq) {
  std::uint64_t m = ~Bit(sq);
  p.w_men &= m;
  p.b_men &= m;
  p.w_king &= m;
  p.b_king &= m;
}

void SetPiece(Pos& p, int sq, PieceBb kind) {
  std::uint64_t b = Bit(sq);
  switch (kind) {
    case PieceBb::WMen:  p.w_men  |= b; break;
    case PieceBb::BMen:  p.b_men  |= b; break;
    case PieceBb::WKing: p.w_king |= b; break;
    case PieceBb::BKing: p.b_king |= b; break;
  }
}

constexpr std::array<int, 3> ManDirs(Color c) {
  return (c == Color::kWhite) ? kWhiteManDirs : kBlackManDirs;
}

// Build via vector from accumulated landings (drop final landing — it's `to`).
std::vector<Square> MakeVia(const std::vector<Square>& landings) {
  if (landings.size() <= 1) return {};
  return std::vector<Square>(landings.begin(), landings.end() - 1);
}

void ManCaptureChain(Pos pos, int sq, Color side, Square origin,
                     std::vector<Square>& landings,
                     std::vector<Square>& captures,
                     std::vector<Move>& out) {
  int r = sq / 8, c = sq % 8;
  int last_rank = (side == Color::kWhite) ? 7 : 0;
  bool extended = false;

  for (int idx : ManDirs(side)) {
    int dr = kDR[idx], dc = kDC[idx];
    int mr = r + dr, mc = c + dc;
    int lr = r + 2 * dr, lc = c + 2 * dc;
    if (!InBoardRC(lr, lc)) continue;
    int msq = SqIdx(mr, mc);
    int lsq = SqIdx(lr, lc);
    if (!IsEnemy(pos, msq, side)) continue;
    if (!IsEmpty(pos, lsq)) continue;

    extended = true;
    Pos next = pos;
    PieceBb kind = PieceAt(next, sq);
    RemoveAt(next, sq);
    RemoveAt(next, msq);
    SetPiece(next, lsq, kind);

    captures.push_back(Square{mr, mc});
    landings.push_back(Square{lr, lc});
    if (lr == last_rank) {
      // Promotion ends sequence: man becomes king, cannot continue this move.
      out.push_back(
          Move{origin, Square{lr, lc}, MakeVia(landings), captures, true});
    } else {
      ManCaptureChain(next, lsq, side, origin, landings, captures, out);
    }
    landings.pop_back();
    captures.pop_back();
  }

  if (!extended && !captures.empty()) {
    out.push_back(
        Move{origin, Square{r, c}, MakeVia(landings), captures, false});
  }
}

void KingCaptureChain(Pos pos, int sq, Color side, Square origin,
                      std::vector<Square>& landings,
                      std::vector<Square>& captures,
                      std::vector<Move>& out) {
  int r = sq / 8, c = sq % 8;
  bool extended = false;

  for (int idx = 0; idx < 4; ++idx) {
    int dr = kDR[idx], dc = kDC[idx];
    int step = 1;
    while (true) {
      int nr = r + dr * step, nc = c + dc * step;
      if (!InBoardRC(nr, nc)) break;
      int nsq = SqIdx(nr, nc);
      if (IsOwn(pos, nsq, side)) break;
      if (IsEnemy(pos, nsq, side)) {
        for (int land = step + 1;; ++land) {
          int lr = r + dr * land, lc = c + dc * land;
          if (!InBoardRC(lr, lc)) break;
          int lsq = SqIdx(lr, lc);
          if (!IsEmpty(pos, lsq)) break;

          extended = true;
          Pos next = pos;
          PieceBb kind = PieceAt(next, sq);
          RemoveAt(next, sq);
          RemoveAt(next, nsq);
          SetPiece(next, lsq, kind);

          captures.push_back(Square{nr, nc});
          landings.push_back(Square{lr, lc});
          KingCaptureChain(next, lsq, side, origin, landings, captures, out);
          landings.pop_back();
          captures.pop_back();
        }
        break;
      }
      ++step;
    }
  }

  if (!extended && !captures.empty()) {
    out.push_back(
        Move{origin, Square{r, c}, MakeVia(landings), captures, false});
  }
}

void GenerateCaptures(const Pos& pos, Color side, std::vector<Move>& out) {
  std::uint64_t own_men = (side == Color::kWhite) ? pos.w_men : pos.b_men;
  std::uint64_t own_king = (side == Color::kWhite) ? pos.w_king : pos.b_king;

  for (std::uint64_t bb = own_men; bb;) {
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    std::vector<Square> landings;
    std::vector<Square> captures;
    ManCaptureChain(pos, sq, side, Square{sq / 8, sq % 8}, landings, captures,
                    out);
  }
  for (std::uint64_t bb = own_king; bb;) {
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    std::vector<Square> landings;
    std::vector<Square> captures;
    KingCaptureChain(pos, sq, side, Square{sq / 8, sq % 8}, landings, captures,
                     out);
  }
}

void GenerateQuiets(const Pos& pos, Color side, std::vector<Move>& out) {
  std::uint64_t occ = AllOcc(pos);
  std::uint64_t own_men = (side == Color::kWhite) ? pos.w_men : pos.b_men;
  std::uint64_t own_king = (side == Color::kWhite) ? pos.w_king : pos.b_king;
  int last_rank = (side == Color::kWhite) ? 7 : 0;

  for (std::uint64_t bb = own_men; bb;) {
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    int r = sq / 8, c = sq % 8;
    for (int idx : ManDirs(side)) {
      int nr = r + kDR[idx], nc = c + kDC[idx];
      if (!InBoardRC(nr, nc)) continue;
      if (occ & BitRC(nr, nc)) continue;
      out.push_back(
          Move{Square{r, c}, Square{nr, nc}, {}, {}, nr == last_rank});
    }
  }
  for (std::uint64_t bb = own_king; bb;) {
    int sq = std::countr_zero(bb);
    bb &= bb - 1;
    int r = sq / 8, c = sq % 8;
    for (int idx = 0; idx < 4; ++idx) {
      for (int step = 1;; ++step) {
        int nr = r + kDR[idx] * step, nc = c + kDC[idx] * step;
        if (!InBoardRC(nr, nc)) break;
        if (occ & BitRC(nr, nc)) break;
        out.push_back(Move{Square{r, c}, Square{nr, nc}, {}, {}, false});
      }
    }
  }
}

}  // namespace

Board::Board() { Setup(); }

void Board::Setup() {
  w_men_ = 0x0000000000FFFF00ULL;
  b_men_ = 0x00FFFF0000000000ULL;
  w_king_ = 0;
  b_king_ = 0;
  side_to_move_ = Color::kWhite;
  halfmove_clock_ = 0;
  history_.clear();
}

void Board::Reset() { Setup(); }

bool Board::InBounds(int row, int col) { return InBoardRC(row, col); }
bool Board::InBounds(const Square& s) { return InBoardRC(s.row, s.col); }

std::optional<PieceKind> Board::At(int row, int col) const {
  if (!InBoardRC(row, col)) return std::nullopt;
  std::uint64_t b = BitRC(row, col);
  if (w_men_ & b) return PieceKind::WMan;
  if (w_king_ & b) return PieceKind::WKing;
  if (b_men_ & b) return PieceKind::BMan;
  if (b_king_ & b) return PieceKind::BKing;
  return PieceKind::Empty;
}

BoardState Board::Snapshot() const {
  return BoardState{w_men_,  b_men_,        w_king_,
                    b_king_, side_to_move_, halfmove_clock_};
}

void Board::Restore(const BoardState& s) {
  w_men_ = s.w_men_;
  b_men_ = s.b_men_;
  w_king_ = s.w_king_;
  b_king_ = s.b_king_;
  side_to_move_ = s.side_to_move;
  halfmove_clock_ = s.halfmove_clock;
}

void Board::Apply(const Move& move) {
  history_.push_back({Snapshot(), move});
  ApplyNoHistory(move);
}

void Board::ApplyNoHistory(const Move& move) {
  int from_sq = SqIdx(move.from.row, move.from.col);
  int to_sq = SqIdx(move.to.row, move.to.col);
  std::uint64_t from_b = Bit(from_sq);
  std::uint64_t to_b = Bit(to_sq);

  std::uint64_t* mover = nullptr;
  if (w_men_ & from_b) mover = &w_men_;
  else if (w_king_ & from_b) mover = &w_king_;
  else if (b_men_ & from_b) mover = &b_men_;
  else if (b_king_ & from_b) mover = &b_king_;
  if (!mover) return;

  bool was_man = (mover == &w_men_) || (mover == &b_men_);

  *mover &= ~from_b;
  *mover |= to_b;

  for (const Square& cap : move.captures) {
    std::uint64_t cb = BitRC(cap.row, cap.col);
    w_men_ &= ~cb;
    w_king_ &= ~cb;
    b_men_ &= ~cb;
    b_king_ &= ~cb;
  }

  if (move.promotes) {
    if (mover == &w_men_) {
      w_men_ &= ~to_b;
      w_king_ |= to_b;
    } else if (mover == &b_men_) {
      b_men_ &= ~to_b;
      b_king_ |= to_b;
    }
  }

  if (was_man || !move.captures.empty()) {
    halfmove_clock_ = 0;
  } else {
    ++halfmove_clock_;
  }

  side_to_move_ =
      (side_to_move_ == Color::kWhite) ? Color::kBlack : Color::kWhite;
}

void Board::Undo() {
  if (history_.empty()) return;
  Restore(history_.back().pre_state);
  history_.pop_back();
}

std::vector<Move> Board::LegalMoves() const {
  Pos pos{w_men_, b_men_, w_king_, b_king_};
  std::vector<Move> captures;
  GenerateCaptures(pos, side_to_move_, captures);

  if (!captures.empty()) {
    std::size_t max_n = 0;
    for (const auto& m : captures) {
      max_n = std::max(max_n, m.captures.size());
    }
    std::vector<Move> filtered;
    filtered.reserve(captures.size());
    for (auto& m : captures) {
      if (m.captures.size() == max_n) filtered.push_back(std::move(m));
    }
    return filtered;
  }

  std::vector<Move> quiet;
  GenerateQuiets(pos, side_to_move_, quiet);
  return quiet;
}

bool Board::IsLegal(const Move& move) const {
  auto moves = LegalMoves();
  for (const auto& m : moves) {
    if (m.from == move.from && m.to == move.to && m.via == move.via &&
        m.captures == move.captures) {
      return true;
    }
  }
  return false;
}

std::optional<GameResult> Board::Result() const {
  std::uint64_t white = w_men_ | w_king_;
  std::uint64_t black = b_men_ | b_king_;
  if (white == 0) return GameResult::kBlackWins;
  if (black == 0) return GameResult::kWhiteWins;
  if (LegalMoves().empty()) {
    return (side_to_move_ == Color::kWhite) ? GameResult::kBlackWins
                                            : GameResult::kWhiteWins;
  }
  if (halfmove_clock_ >= 80) return GameResult::kDraw;
  return std::nullopt;
}

GameResult Board::HandleResign() const {
  return (side_to_move_ == Color::kWhite) ? GameResult::kBlackWins
                                          : GameResult::kWhiteWins;
}
