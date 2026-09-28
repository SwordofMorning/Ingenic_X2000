root.CXXFLAGS += -Ithird_party/libcxx/include/
root.CXXFLAGS += -D_LIBCPP_BUILDING_LIBRARY
root.CXXFLAGS += -D_LIBCPP_NO_EXCEPTIONS
root.CXXFLAGS += -nostdinc++

ifndef APP_libmcu_libcxx_thread
root.CXXFLAGS += -D_LIBCPP_HAS_NO_THREADS
endif

ifndef APP_libmcu_libcxx_rtti
root.CXXFLAGS += -fno-rtti
endif

src-y += src/algorithm.cpp
src-y += src/any.cpp
src-y += src/bind.cpp
src-y += src/charconv.cpp
src-n += src/chrono.cpp

src-y += src/condition_variable.cpp
src-y += src/condition_variable_destructor.cpp
# src-y += src/debug.cpp
src-y += src/exception.cpp
# src-y += src/experimental
# src-y += src/filesystem
src-y += src/functional.cpp
src-y += src/future.cpp
src-y += src/hash.cpp
src-y += src/ios.cpp
src-y += src/iostream.cpp
src-y += src/locale.cpp
src-y += src/memory.cpp
src-y += src/mutex.cpp
src-y += src/mutex_destructor.cpp
src-y += src/new.cpp
src-y += src/optional.cpp
src-y += src/random.cpp
src-y += src/regex.cpp
# src-y += src/shared_mutex.cpp
src-y += src/stdexcept.cpp
src-y += src/string.cpp
src-y += src/strstream.cpp
src-y += src/cxxabi.cpp
src-y += src/system_error.cpp
src-y += src/thread.cpp
# 使用 libcxxrt 的 typeinfo 和 dynamic_cast 支持 typeid
# src-y += src/typeinfo.cpp
src-$(APP_libmcu_libcxx_rtti) += src/typeinfo_rt.cpp
src-$(APP_libmcu_libcxx_rtti) += src/dynamic_cast_rt.cpp
src-y += src/utility.cpp
src-y += src/valarray.cpp
src-y += src/variant.cpp
src-y += src/vector.cpp
