#include "foobar2000.h"
#include "foosort.h"

#define FOOSORT_PROFILING 0

namespace {
#if FOOSORT_PROFILING
	typedef pfc::hires_timer foosort_timer;
#endif

	class foosort {
	public:
		foosort(pfc::sort_callback & cb, abort_callback & a) : m_abort(a), p_callback(cb) {}

		foosort fork() {
			foosort ret ( * this );
			ret.genrand = genrand_service::g_create();
			return ret;
		}

		size_t myrand(size_t cnt) {
			return genrand->genrand(cnt);
		}

		void squaresort(t_size const p_base, t_size const p_count) {
			const t_size max = p_base + p_count;
			for (t_size walk = p_base + 1; walk < max; ++walk) {
				for (t_size prev = p_base; prev < walk; ++prev) {
					p_callback.swap_check(prev, walk);
				}
			}
		}


		void _sort_2elem_helper(t_size & p_elem1, t_size & p_elem2) {
			if (p_callback.compare(p_elem1, p_elem2) > 0) pfc::swap_t(p_elem1, p_elem2);
		}

		t_size _pivot_helper(t_size const p_base, t_size const p_count) {
			PFC_ASSERT(p_count > 2);

			//t_size val1 = p_base, val2 = p_base + (p_count / 2), val3 = p_base + (p_count - 1);

			t_size val1 = myrand(p_count), val2 = myrand(p_count - 1), val3 = myrand(p_count - 2);
			if (val2 >= val1) val2++;
			if (val3 >= val1) val3++;
			if (val3 >= val2) val3++;

			val1 += p_base; val2 += p_base; val3 += p_base;

			_sort_2elem_helper(val1, val2);
			_sort_2elem_helper(val1, val3);
			_sort_2elem_helper(val2, val3);

			return val2;
		}

		void newsort(t_size const p_base, t_size const p_count, size_t concurrency) {
			if (p_count <= 4) {
				squaresort(p_base, p_count);
				return;
			}
			
			this->m_abort.check();
#if FOOSORT_PROFILING
			foosort_timer t;
			if ( concurrency > 1 ) t.start();
#endif

			t_size pivot = _pivot_helper(p_base, p_count);

			{
				const t_size target = p_base + p_count - 1;
				if (pivot != target) {
					p_callback.swap(pivot, target); pivot = target;
				}
			}


			t_size partition = p_base;
			{
				bool asdf = false;
				for (t_size walk = p_base; walk < pivot; ++walk) {
					const int comp = p_callback.compare(walk, pivot);
					bool trigger = false;
					if (comp == 0) {
						trigger = asdf;
						asdf = !asdf;
					} else if (comp < 0) {
						trigger = true;
					}
					if (trigger) {
						if (partition != walk) p_callback.swap(partition, walk);
						partition++;
					}
				}
			}
			if (pivot != partition) {
				p_callback.swap(pivot, partition); pivot = partition;
			}

			const auto base1 = p_base, count1 = pivot - p_base;
			const auto base2 = pivot + 1, count2 = p_count - (pivot + 1 - p_base);

			if (concurrency > 1) {

				size_t total = count1 + count2;
				size_t con1 = (size_t)((uint64_t)count1 * (uint64_t)concurrency / (uint64_t)total);
				if (con1 < 1) con1 = 1;
				if (con1 > concurrency - 1) con1 = concurrency - 1;
				size_t con2 = concurrency - con1;

#if FOOSORT_PROFILING
				FB2K_console_formatter() << "foosort pass: " << p_base << "+" << p_count << "(" << concurrency << ") took " << t.queryString();
				FB2K_console_formatter() << "foosort forking: " << base1 << "+" << count1 << "(" << con1 << ") + " << base2 << "+" << count2 << "(" << con2 << ")";
#endif
				pfc::thread2 other;
				other.startHere([&] {
					try {
						foosort subsort = fork();
						subsort.newsort(base1, count1, con1);
					} catch (exception_aborted) {}
				});
				try {
					newsort(base2, count2, con2);
				} catch (exception_aborted) {}
				other.waitTillDone();
				m_abort.check();
			} else {
				newsort(base1, count1, 1);
				newsort(base2, count2, 1);
			}
		}
	private:
		abort_callback & m_abort;
		pfc::sort_callback & p_callback;
		genrand_service::ptr genrand = genrand_service::g_create();
	};



}
namespace fb2k {
	void sort(pfc::sort_callback & cb, size_t count, size_t concurrency, abort_callback & aborter) {
#if FOOSORT_PROFILING
		foosort_timer t; t.start();
#endif
		foosort theFooSort(cb, aborter);
		theFooSort.newsort(0, count, concurrency);
#if FOOSORT_PROFILING
		FB2K_console_formatter() << "foosort took: " << t.queryString();
#endif
	}
}
