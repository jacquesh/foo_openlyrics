#pragma once

namespace fb2k {
	class playlistColumnProvider : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(playlistColumnProvider);
	public:
		virtual size_t numColumns() = 0;
		virtual GUID columnID(size_t col) = 0;
		virtual fb2k::stringRef columnFormatSpec(size_t col) = 0;
		virtual fb2k::stringRef columnSortScript(size_t col) = 0;
		virtual fb2k::stringRef columnName(size_t col) = 0;
		virtual unsigned columnFlags(size_t col) = 0;

		static constexpr unsigned flag_alignLeft = 0;
		static constexpr unsigned flag_alignRight = 1 << 0;
        static constexpr unsigned flag_alignCenter = 1 << 1;
	};
}
