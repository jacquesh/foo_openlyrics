#pragma once

class CDialogResizeHelperCompat {
public:
	struct param {
		unsigned short id;
		unsigned short flags;
	};

	enum {
		X_MOVE = 1, X_SIZE = 2, Y_MOVE = 4, Y_SIZE = 8,
		XY_MOVE = X_MOVE | Y_MOVE, XY_SIZE = X_SIZE | Y_SIZE,
		X_MOVE_Y_SIZE = X_MOVE | Y_SIZE, X_SIZE_Y_MOVE = X_SIZE | Y_MOVE,
	};

};