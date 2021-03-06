#include <asio_cares/process_one.hpp>

#include <ares.h>
#include <asio_cares/channel.hpp>
#include <asio_cares/done.hpp>
#include <asio_cares/error.hpp>
#include <asio_cares/library.hpp>
#include <asio_cares/string.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include "setup.hpp"
#include <catch.hpp>

#ifdef _WIN32
#include <nameser.h>
#else
#include <arpa/nameser.h>
#endif

namespace asio_cares {
namespace tests {
namespace {

SCENARIO("asio_cares::async_process_one may be used to incrementally and asynchronously complete a DNS query", "[asio_cares][process_one]") {
	GIVEN("An asio_cares::channel") {
		library l;
		boost::asio::io_service ios;
		channel c(ios);
		setup(c);
		WHEN("A query is sent thereupon") {
			unsigned char * ptr;
			int buflen;
			int result = ares_create_query("google.com",
				                           ns_c_any,
				                           ns_t_a,
				                           0,
				                           1,
				                           &ptr,
				                           &buflen,
				                           0);
			raise(result);
			string g(ptr);
			class state {
			public:
				state () noexcept
					:	invoked(false)
				{}
				boost::system::error_code error_code;
				bool                      invoked;
			};
			state s;
			auto f = [] (void * data, int status, int, unsigned char *, int) noexcept {
				auto && s = *static_cast<state *>(data);
				s.invoked = true;
				s.error_code = asio_cares::make_error_code(status);
			};
			ares_send(c, ptr, buflen, f, &s);
			REQUIRE_FALSE(s.invoked);
			REQUIRE_FALSE(done(c));
			AND_WHEN("asio_cares::async_process_one is invoked until asio_cares::done reports that the query has completed") {
				bool done = false;
				do {
					boost::system::error_code ec;
					bool invoked = false;
					async_process_one(c, [&] (auto e, auto d) noexcept {
						ec = e;
						done = d;
						invoked = true;
					});
					ios.run();
					REQUIRE(invoked);
					INFO(ec.message());
					REQUIRE_FALSE(ec);
					ios.reset();
				} while (!done);
				THEN("The query completes successfully") {
					REQUIRE(s.invoked);
					INFO(s.error_code.message());
					CHECK_FALSE(s.error_code);
				}
			}
		}
		WHEN("asio_cares::async_process_one is invoked thereupon") {
			boost::system::error_code ec;
			bool invoked = false;
			bool done = false;
			async_process_one(c, [&] (auto e, auto d) noexcept {
				ec = e;
				invoked = true;
				done = d;
			});
			ios.run();
			THEN("The operation completes successfully") {
				REQUIRE(invoked);
				INFO(ec.message());
				REQUIRE_FALSE(ec);
				CHECK(done);
			}
		}
	}
}

}
}
}
