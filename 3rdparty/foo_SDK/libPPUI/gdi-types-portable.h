#pragma once
#include <utility> // std::swap


namespace gdi_types_portable {

	template<typename val_t>
	struct CSize {
		typedef CSize<val_t> self_t;

		CSize() {}
		CSize(val_t _cx, val_t _cy) : cx(_cx), cy(_cy) {}

		val_t cx = 0, cy = 0;
	};

	template<typename val_t>
	struct CPoint {
		typedef CPoint<val_t> self_t;
		typedef CSize<val_t> size_t;

		CPoint() {}
		CPoint(val_t _x, val_t _y) : x(_x), y(_y) {}

		self_t operator+(self_t const & other) const {
			return self_t(x + other.x, y + other.y);
		}
		self_t operator-(self_t const & other) const {
			return self_t(x - other.x, y - other.y);
		}
		static bool Equals(const self_t & v1, const self_t & v2) {return v1.x == v2.x && v1.y == v2.y;}
		bool operator==(const self_t & other) const { return Equals(*this, other); }
		bool operator!=(const self_t & other) const { return !Equals(*this, other); }


		val_t x = 0, y = 0;
	};

	template<typename val_t>
	struct CRect {

		typedef CRect<val_t> self_t;
		typedef CPoint<val_t> point_t;
		typedef CSize<val_t> size_t;

		CRect() {
			left = 0, top = 0, right = 0, bottom = 0;
			static_assert(sizeof(self_t) == 2 * sizeof(point_t), "sanity");
		}
		CRect(val_t l, val_t t, val_t r, val_t b) : left(l), top(t), right(r), bottom(b) {}
		CRect(point_t p1, point_t p2) {
			_topLeft = p1; _bottomRight = p2;
			this->NormalizeRect();
		}

		union {
			struct {
				val_t left, top, right, bottom;
			};
			struct {
				point_t _topLeft, _bottomRight;
			};
		};


		point_t & TopLeft() { return _topLeft; }
		point_t & BottomRight() { return _bottomRight; }
		const point_t & TopLeft() const { return _topLeft; }
		const point_t & BottomRight() const { return _bottomRight; }

		size_t Size() const {
			return size_t(Width(), Height());
		}

		val_t Width() const { return right - left; }
		val_t Height() const { return bottom - top; }

		void NormalizeRect() {
			if (right < left) std::swap(left, right);
			if (bottom < top) std::swap(top, bottom);
		}
		point_t CenterPoint() const {
			return point_t((left + right) / 2, (top + bottom) / 2);
		}
	};
}
