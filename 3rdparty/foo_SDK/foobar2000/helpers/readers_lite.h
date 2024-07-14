#pragma once

file::ptr createFileWithMemBlock(fb2k::memBlock::ptr mem, t_filestats stats = filestats_invalid, const char* contentType = nullptr, bool remote = false);
file::ptr createFileLimited(file::ptr base, t_filesize offset, t_filesize size, abort_callback& abort);
file::ptr createFileBigMemMirror(file::ptr source, abort_callback& abort);
file::ptr createFileMemMirror(file::ptr source, abort_callback& abort);
file::ptr createFileMemMirrorAsync(file::ptr source, completion_notify::ptr optionalDoneReading, abort_callback & a);
