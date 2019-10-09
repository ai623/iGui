#pragma once

namespace iGui {
	template <typename T>
	struct Position {
		T x;
		T y;
	};

	template <typename T>
	struct Rect {
		T width;
		T height;
	};
}