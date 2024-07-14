#include "foobar2000-sdk-pch.h"


t_filestats2 fsItemBase::getStatsOpportunist() {
	return filestats2_invalid;
}

void fsItemBase::remove(abort_callback& aborter) {
	getFS()->remove(this->canonicalPath()->c_str(), aborter);
}

fb2k::stringRef fsItemBase::shortName() {
	return nameWithExt();
}

file::ptr fsItemFile::openRead(abort_callback& aborter) {
	return open(filesystem::open_mode_read, aborter);
}
file::ptr fsItemFile::openWriteExisting(abort_callback& aborter) {
	return open(filesystem::open_mode_write_existing, aborter);
}
file::ptr fsItemFile::openWriteNew(abort_callback& aborter) {
	return open(filesystem::open_mode_write_new, aborter);
}

void fsItemFolder::removeRecur(abort_callback& aborter) {
	auto list = this->listContents(listMode::filesAndFolders | listMode::hidden | listMode::suppressStats, aborter);
	for (auto i : list->typed< fsItemBase >()) {
		fsItemFolderPtr f;
		if (f &= i) f->removeRecur(aborter);
		else i->remove(aborter);
	}
	
	this->remove(aborter);
}

void fsItemFile::copyToOther(fsItemFilePtr other, abort_callback& aborter) {
	aborter.check();

	auto fSource = this->openRead(aborter);
	auto fTarget = other->openWriteNew(aborter);
	file::g_transfer_file(fSource, fTarget, aborter);

	// fTarget->commit(aborter);

	// we cannot transfer other properties, this should be overridden for such
}

fsItemPtr fsItemBase::copyTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter) {
	aborter.check();
	{
		fsItemFilePtr f;
		if (f &= this) {
			auto target = folder->createFile(desiredName, createMode, aborter);
			f->copyToOther(target, aborter);
			return std::move(target);
		}
	}
	{
		fsItemFolderPtr f;
		if (f &= this) {
			auto target = folder->createFolder(desiredName, createMode, aborter);
			auto contents = f->listContents(listMode::filesAndFolders | listMode::hidden | listMode::suppressStats, aborter);
			for (auto item : contents->typed<fsItemBase>()) {
				item->copyTo(target, createMode, aborter);
			}
			return std::move(target);
		}
	}
	PFC_ASSERT(!"Should not get here, bad fsItemBase object");
	return nullptr;
}

fsItemPtr fsItemBase::copyTo(fsItemFolderPtr folder, unsigned createMode, abort_callback& aborter) {
	auto temp = pfc::string_filename_ext(this->canonicalPath()->c_str());
	if (temp.length() == 0) throw pfc::exception_invalid_params();
	return this->copyTo(folder, temp.c_str(), createMode, aborter);
}

fsItemPtr fsItemBase::moveTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter) {
	auto ret = this->copyTo(folder, desiredName, createMode, aborter);
	fsItemFolder::ptr asFolder;
	if (asFolder &= this) {
		asFolder->removeRecur(aborter);
	} else {
		this->remove(aborter);
	}
	return ret;
}

fsItemPtr fsItemBase::moveTo(fsItemFolderPtr folder, unsigned createMode, abort_callback& aborter) {
	auto fn = this->nameWithExt();
	return this->moveTo(folder, fn->c_str(), createMode, aborter);
}
fb2k::memBlockRef fsItemFile::readWhole(size_t sizeSanity, abort_callback& aborter) {
	auto file = this->openRead(aborter);
	auto size64 = file->get_size_ex(aborter);
	if (size64 > sizeSanity) throw exception_io_data();
	auto size = (size_t)size64;
	pfc::mem_block block;
	block.resize(size);
	file->read_object(block.ptr(), size, aborter);
	return fb2k::memBlock::blockWithData(std::move(block));
}

namespace {
	static void uniqueFn(pfc::string8& fn, unsigned add) {
		if (add > 0) {
			auto fnOnly = pfc::string_filename(fn);
			auto ext = pfc::string_extension(fn);
			fn.reset();
			fn << fnOnly << " (" << add << ")";
			if (ext.length() > 0) fn << "." << ext.get_ptr();
		}
	}

	using namespace fb2k;
	class fsItemFileStd : public fsItemFile {
	public:
		fsItemFileStd(filesystem::ptr fs, stringRef canonicalPath, t_filestats2 const & opportunistStats) : m_fs(fs), m_path(canonicalPath), m_opportunistStats(opportunistStats) {
			m_opportunistStats.set_folder(false);
			m_opportunistStats.set_remote(fs->is_remote(canonicalPath->c_str()));
		}

		filesystem::ptr getFS() override { return m_fs; }

		bool isRemote() override { return m_fs->is_remote(m_path->c_str()); }

		t_filestats2 getStats2(uint32_t s2flags, abort_callback& a) override {
			auto s = m_fs->get_stats2_(m_path->c_str(), s2flags, a);
			PFC_ASSERT((s2flags & stats2_fileOrFolder) == 0 || !s.is_folder());
			return s;
		}

		fb2k::stringRef canonicalPath() override { return m_path; }

		fb2k::stringRef nameWithExt() override {
			pfc::string8 temp;
			m_fs->extract_filename_ext(m_path->c_str(), temp);
			return makeString(temp);
		}
		fb2k::stringRef shortName() override {
			pfc::string8 temp;
			if (m_fs->get_display_name_short_(m_path->c_str(), temp)) return makeString(temp);
			return nameWithExt();
		}
		t_filestats2 getStatsOpportunist() override {
			return m_opportunistStats;
		}
		file::ptr open(uint32_t openMode, abort_callback& aborter) override {
			file::ptr ret;
			m_fs->open(ret, m_path->c_str(), openMode, aborter);
			return ret;
		}
		fb2k::memBlockRef readWhole(size_t sizeSanity, abort_callback& aborter) override {
			// Prefer fs->readWholeFile over fsItemFile methods as the fs object might implement it
			return m_fs->readWholeFile(m_path->c_str(), sizeSanity, aborter);
		}
		fsItemPtr moveTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter) override {
			for (unsigned add = 0; ; ++add) {
				pfc::string8 fn(desiredName); uniqueFn(fn, add);
				pfc::string8 dst(folder->canonicalPath()->c_str());
				dst.end_with(m_fs->pathSeparator());
				dst += fn;
				bool bDidExist = false;
				try {
					if (createMode == createMode::allowExisting) m_fs->move_overwrite(m_path->c_str(), dst, aborter);
					else m_fs->move(m_path->c_str(), dst, aborter);
				} catch (exception_io_already_exists) {
					bDidExist = true;
				}

				switch (createMode) {
				case createMode::allowExisting:
					break; // OK
				case createMode::failIfExists:
					if (bDidExist) throw exception_io_already_exists();
					break; // OK
				case createMode::generateUniqueName:
					if (bDidExist) {
						continue; // the for loop
					}
					break; // OK
				default:
					PFC_ASSERT(!"Should not get here");
					break;
				}
				auto stats = m_opportunistStats;
				stats.set_file();
				return m_fs->makeItemFileStd(dst, stats);
			}
		}
	private:
		const filesystem::ptr m_fs;
		const stringRef m_path;
		t_filestats2 m_opportunistStats;
	};
	class fsItemFolderStd : public fsItemFolder {
	public:
		fsItemFolderStd(filesystem::ptr fs, stringRef canonicalPath, t_filestats2 const & opportunistStats) : m_fs(fs), m_path(canonicalPath), m_opportunistStats(opportunistStats) {
			m_opportunistStats.set_folder(true);
			m_opportunistStats.set_remote(fs->is_remote(canonicalPath->c_str()));
		}

		filesystem::ptr getFS() override { return m_fs; }

		t_filestats2 getStatsOpportunist() override {
			return m_opportunistStats;
		}

		bool isRemote() override { return m_fs->is_remote(m_path->c_str()); }

		t_filestats2 getStats2(uint32_t s2flags, abort_callback& a) override {
			auto s = m_fs->get_stats2_(m_path->c_str(), s2flags, a);
			PFC_ASSERT((s2flags & stats2_fileOrFolder) == 0 || s.is_folder() );
			return s;
		}

		fb2k::stringRef canonicalPath() override { return m_path; }

		fb2k::stringRef nameWithExt() override {
			pfc::string8 temp;
			m_fs->extract_filename_ext(m_path->c_str(), temp);
			return makeString(temp);
		}
		fb2k::stringRef shortName() override {
			pfc::string8 temp;
			if (m_fs->get_display_name_short_(m_path->c_str(), temp)) return makeString(temp);
			return nameWithExt();
		}
		fb2k::arrayRef listContents(unsigned listMode, abort_callback& aborter) override {
			auto out = arrayMutable::empty();
			filesystem::list_callback_t cb = [&] ( const char * p, t_filestats2 const & stats ) {
				if (stats.is_folder()) {
					if (listMode & listMode::folders) out->add(new service_impl_t< fsItemFolderStd >(m_fs, makeString(p), stats));
				} else {
					if (listMode & listMode::files) out->add(new service_impl_t< fsItemFileStd >(m_fs, makeString(p), stats));
				}
			};
			m_fs->list_directory_(m_path->c_str(), cb, listMode, aborter);
			return out->copyConst();
		}

		fsItemFile::ptr findChildFile(const char* fileName, abort_callback& aborter) override {
			auto sub = subPath(fileName);
			auto stats = m_fs->get_stats2_(sub->c_str(), stats2_fileOrFolder | foobar2000_io::stats2_remote, aborter);
			if (!stats.is_folder()) {
				return m_fs->makeItemFileStd(sub->c_str(), stats);
			}
			throw exception_io_not_found();
		}
		fsItemFolder::ptr findChildFolder(const char* fileName, abort_callback& aborter) override {
			auto sub = subPath(fileName);
			auto stats = m_fs->get_stats2_(sub->c_str(), stats2_fileOrFolder | foobar2000_io::stats2_remote, aborter);
			if (stats.is_folder()) {
				return m_fs->makeItemFolderStd(sub->c_str(), stats);
			}
			throw exception_io_not_found();
		}
		fsItemBase::ptr findChild(const char* fileName, abort_callback& aborter) override {
			auto sub = subPath(fileName);
			auto stats = m_fs->get_stats2_(sub->c_str(), stats2_fileOrFolder | foobar2000_io::stats2_remote, aborter);
			if ( stats.is_folder() ) {
				return m_fs->makeItemFileStd(sub->c_str(), stats );
			} else {
				return m_fs->makeItemFolderStd(sub->c_str(), stats );
			}
		}
		fsItemFile::ptr createFile(const char* fileName, unsigned createMode, abort_callback& aborter) override {
			for (unsigned add = 0; ; ++add) {
				pfc::string8 fn(fileName); uniqueFn(fn, add);
				auto sub = subPath(fn);

				t_filestats2 stats;

				bool bDidExist = false;
				try {
					stats = m_fs->get_stats2_( sub->c_str(), stats2_all, aborter );
					bDidExist = stats.is_file();
				} catch(exception_io_not_found) {}
				switch (createMode) {
				case createMode::allowExisting:
					break; // OK
				case createMode::failIfExists:
					if (bDidExist) throw exception_io_already_exists();
					break; // OK
				case createMode::generateUniqueName:
					if (bDidExist) {
						continue; // the for loop
					}
					break;
				default:
					PFC_ASSERT(!"Should not get here");
					break;
				}
				if (!bDidExist) {
					// actually create an empty file if it did not yet exist
					// FIX ME this should be atomic with exists() check
					file::ptr creator;
					m_fs->open(creator, sub->c_str(), filesystem::open_mode_write_new, aborter);
					stats = creator->get_stats2_( stats2_all, aborter );
				}
				return m_fs->makeItemFileStd(sub->c_str(), stats);
			}
		}
		fsItemFolder::ptr createFolder(const char* fileName, unsigned createMode, abort_callback& aborter) override {
			for (unsigned add = 0; ; ++add) {
				pfc::string8 fn(fileName); uniqueFn(fn, add);
				auto sub = subPath(fn);
				bool bDidExist = false;
				try {
					m_fs->create_directory(sub->c_str(), aborter);
				} catch (exception_io_already_exists) {
					bDidExist = true;
				}
				switch (createMode) {
				case createMode::allowExisting:
					break; // OK
				case createMode::failIfExists:
					if (bDidExist) throw exception_io_already_exists();
					break;
				case createMode::generateUniqueName:
					if (bDidExist) {
						continue; // the for loop
					}
					break;
				default:
					PFC_ASSERT(!"Should not get here");
					break;
				}
				// Inherit opportunist stats
				return m_fs->makeItemFolderStd(sub->c_str(), this->m_opportunistStats);
			}
		}
		fsItemPtr moveTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter) override {
			for (unsigned add = 0; ; ++add) {
				pfc::string8 fn(desiredName); uniqueFn(fn, add);
				pfc::string8 dst(folder->canonicalPath()->c_str());
				dst.end_with(m_fs->pathSeparator());
				dst += fn;
				bool bDidExist = false;
				try {
					if (createMode == createMode::allowExisting) m_fs->move_overwrite(m_path->c_str(), dst, aborter);
					else m_fs->move(m_path->c_str(), dst, aborter);
				} catch (exception_io_already_exists) {
					bDidExist = true;
				}

				switch (createMode) {
				case createMode::allowExisting:
					break; // OK
				case createMode::failIfExists:
					if (bDidExist) throw exception_io_already_exists();
					break; // OK
				case createMode::generateUniqueName:
					if (bDidExist) {
						continue; // the for loop
					}
					break; // OK
				default:
					PFC_ASSERT(!"Should not get here");
					break;
				}

				return m_fs->makeItemFolderStd(dst, m_opportunistStats);

			}
		}
	private:
		stringRef subPath(const char* name) {
			pfc::string8 temp(m_path->c_str());
			temp.add_filename(name);
			return makeString(temp);
		}
		const filesystem::ptr m_fs;
		const stringRef m_path;
		t_filestats2 m_opportunistStats;
	};
}

fsItemFolder::ptr filesystem::makeItemFolderStd(const char* pathCanonical, t_filestats2 const& opportunistStats) {
	return new service_impl_t<fsItemFolderStd>(this, makeString(pathCanonical), opportunistStats);
}

fsItemFile::ptr filesystem::makeItemFileStd(const char* pathCanonical, t_filestats2 const & opportunistStats) {
	return new service_impl_t<fsItemFileStd>(this, makeString(pathCanonical), opportunistStats);
}
fsItemBase::ptr filesystem::findItem_(const char* path, abort_callback& p_abort) {
	filesystem_v3::ptr v3;
	if (v3 &= this) {
		return v3->findItem(path, p_abort);
	}
	auto stats = this->get_stats2_(path, stats2_fileOrFolder, p_abort);
	if (stats.is_folder()) return this->makeItemFolderStd(path, stats);
	else return this->makeItemFileStd(path, stats);

}
fsItemFile::ptr filesystem::findItemFile_(const char* path, abort_callback& p_abort) {
	filesystem_v3::ptr v3;
	if (v3 &= this) return v3->findItemFile(path, p_abort);

	auto stats = this->get_stats2_(path, stats2_fileOrFolder, p_abort);
	if (!stats.is_folder()) return this->makeItemFileStd(path, stats);
	throw exception_io_not_found();
}
fsItemFolder::ptr filesystem::findItemFolder_(const char* path, abort_callback& p_abort) {
	filesystem_v3::ptr v3;
	if (v3 &= this) return v3->findItemFolder(path, p_abort);

	auto stats = this->get_stats2_(path, stats2_fileOrFolder, p_abort);
	if (stats.is_folder()) return this->makeItemFolderStd(path, stats);
	throw exception_io_not_found();
}


fsItemBase::ptr fsItemBase::fromPath(const char* path, abort_callback& aborter) {
	return filesystem::get(path)->findItem_(path, aborter);
}

fsItemFile::ptr fsItemFile::fromPath(const char* path, abort_callback& aborter) {
	return filesystem::get(path)->findItemFile_(path, aborter);
}

fsItemFolder::ptr fsItemFolder::fromPath(const char* path, abort_callback& aborter) {
	return filesystem::get(path)->findItemFolder_(path, aborter);
}

t_filestats fsItemBase::getStats(abort_callback& a) {
	return getStats2(stats2_all, a).to_legacy();
}
