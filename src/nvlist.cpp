#include "zfspp.h"
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/nvpair.h>
#include <sys/stdtypes.h>
#include <system_error>

namespace zfspp {

	const char* nv_type_name(nv_type dt) noexcept {
		static constexpr const char* types[] = {
			"unknown",		 "boolean",		"byte",			"int16",		 "uint16",		 "int32",
			"uint32",		 "int64",		"uint64",		"string",		 "byte_array",	 "int16_array",
			"uint16_array",	 "int32_array", "uint32_array", "int64_array",	 "uint64_array", "string_array",
			"hrtime",		 "nvlist",		"nvlist_array", "boolean_value", "int8",		 "uint8",
			"boolean_array", "int8_array",	"uint88_array",
		};
		if (static_cast<size_t>(dt) >= sizeof(types)/sizeof(types[0])) return types[0];
		return types[static_cast<size_t>(dt)];
	}

	std::string nv_pair::key() const noexcept { return m_pair ? nvpair_name(m_pair) : ""; }

	nv_type nv_pair::type() const noexcept { return static_cast<nv_type>(nvpair_type(m_pair)); }

	template<typename T>
	static T nvpas(nvpair_t* list, int (*fn)(nvpair_t*, T*)) {
		T result{};
		if (list == nullptr || fn(list, &result) != 0) throw std::invalid_argument("invalid type");
		return result;
	}

	template<typename T, typename T2>
	static std::vector<T> nvpasa(nvpair_t* list, int (*fn)(nvpair_t*, T2**, uint*)) {
		T2* result{};
		uint size{};
		if (list == nullptr || fn(list, &result, &size) != 0) throw std::invalid_argument("invalid type");
		std::vector<T> res;
		res.reserve(size);
		for (uint i = 0; i < size; i++) {
			res.emplace_back(result[i]);
		}
		return res;
	}

	bool nv_pair::as_boolean() const {
		if (type() == nv_type::boolean) return true;
		return nvpas(m_pair, &nvpair_value_boolean_value) == B_TRUE;
	}
	uchar_t nv_pair::as_byte() const { return nvpas(m_pair, &nvpair_value_byte); }
	int8_t nv_pair::as_int8() const { return nvpas(m_pair, &nvpair_value_int8); }
	uint8_t nv_pair::as_uint8() const { return nvpas(m_pair, &nvpair_value_uint8); }
	int16_t nv_pair::as_int16() const { return nvpas(m_pair, &nvpair_value_int16); }
	uint16_t nv_pair::as_uint16() const { return nvpas(m_pair, &nvpair_value_uint16); }
	int32_t nv_pair::as_int32() const { return nvpas(m_pair, &nvpair_value_int32); }
	uint32_t nv_pair::as_uint32() const { return nvpas(m_pair, &nvpair_value_uint32); }
	int64_t nv_pair::as_int64() const { return nvpas(m_pair, &nvpair_value_int64); }
	uint64_t nv_pair::as_uint64() const { return nvpas(m_pair, &nvpair_value_uint64); }
	std::string nv_pair::as_string() const { return nvpas(m_pair, &nvpair_value_string); }
	nv_list nv_pair::as_nvlist() const { return nvpas(m_pair, &nvpair_value_nvlist); }
	std::vector<bool> nv_pair::as_boolean_array() const {
		if (type() == nv_type::boolean) return {true};
		return nvpasa<bool>(m_pair, &nvpair_value_boolean_array);
	}
	std::vector<uchar_t> nv_pair::as_byte_array() const { return nvpasa<uchar_t>(m_pair, &nvpair_value_byte_array); }
	std::vector<int8_t> nv_pair::as_int8_array() const { return nvpasa<int8_t>(m_pair, &nvpair_value_int8_array); }
	std::vector<uint8_t> nv_pair::as_uint8_array() const { return nvpasa<uint8_t>(m_pair, &nvpair_value_uint8_array); }
	std::vector<int16_t> nv_pair::as_int16_array() const { return nvpasa<int16_t>(m_pair, &nvpair_value_int16_array); }
	std::vector<uint16_t> nv_pair::as_uint16_array() const {
		return nvpasa<uint16_t>(m_pair, &nvpair_value_uint16_array);
	}
	std::vector<int32_t> nv_pair::as_int32_array() const { return nvpasa<int32_t>(m_pair, &nvpair_value_int32_array); }
	std::vector<uint32_t> nv_pair::as_uint32_array() const {
		return nvpasa<uint32_t>(m_pair, &nvpair_value_uint32_array);
	}
	std::vector<int64_t> nv_pair::as_int64_array() const { return nvpasa<int64_t>(m_pair, &nvpair_value_int64_array); }
	std::vector<uint64_t> nv_pair::as_uint64_array() const {
		return nvpasa<uint64_t>(m_pair, &nvpair_value_uint64_array);
	}
	std::vector<std::string> nv_pair::as_string_array() const {
		return nvpasa<std::string>(m_pair, &nvpair_value_string_array);
	}
	std::vector<nv_list> nv_pair::as_nvlist_array() const {
		return nvpasa<nv_list>(m_pair, &nvpair_value_nvlist_array);
	}

	nv_pair& nv_pair::operator++() noexcept {
		if (m_pair) { m_pair = nvlist_next_nvpair(m_list, m_pair); }
		return *this;
	}

	nv_list::nv_list() : m_handle(nullptr) {}

	nv_list::nv_list(::nvlist* list) : m_handle(nullptr) {
		if (list) {
			if (nvlist_dup(list, &m_handle, 0) != 0) throw std::bad_alloc();
		}
	}

	nv_list::nv_list(const nv_list& other) : m_handle(nullptr) {
		if (other.m_handle) {
			if (nvlist_dup(other.m_handle, &m_handle, 0) != 0) throw std::bad_alloc();
		}
	}

	nv_list::nv_list(nv_list&& other) : m_handle(other.m_handle) { other.m_handle = nullptr; }

	nv_list& nv_list::operator=(const nv_list& other) {
		::nvlist* new_handle{};
		if (other.m_handle) {
			if (nvlist_dup(other.m_handle, &new_handle, 0) != 0) throw std::bad_alloc();
		}
		clear();
		m_handle = new_handle;
		return *this;
	}

	nv_list& nv_list::operator=(nv_list&& other) {
		clear();
		m_handle = other.m_handle;
		other.m_handle = nullptr;
		return *this;
	}

	nv_list::~nv_list() { clear(); }

	void nv_list::ensure_allocated() {
		if (m_handle) return;
		if (nvlist_alloc(&m_handle, NV_UNIQUE_NAME, 0) != 0) throw std::bad_alloc();
	}

	void nv_list::clear() {
		if (m_handle) nvlist_free(m_handle);
		m_handle = nullptr;
	}

	size_t nv_list::size() const noexcept {
		size_t res{};
		if (m_handle == nullptr) return res;
		auto pair = nvlist_next_nvpair(m_handle, nullptr);
		while (pair) {
			res++;
			pair = nvlist_next_nvpair(m_handle, pair);
		}
		return res;
	}

	std::set<std::string> nv_list::keys() const {
		std::set<std::string> res;
		if (m_handle == nullptr) return res;
		auto pair = nvlist_next_nvpair(m_handle, nullptr);
		while (pair) {
			res.insert(nvpair_name(pair));
			pair = nvlist_next_nvpair(m_handle, pair);
		}
		return res;
	}

	nv_pair nv_list::begin() const noexcept {
		if (m_handle == nullptr) return {m_handle, nullptr};
		return nv_pair(m_handle, nvlist_next_nvpair(m_handle, nullptr));
	}

	nv_pair nv_list::find(const char* key) const noexcept {
		if (m_handle == nullptr) return {m_handle, nullptr};
		nvpair* res{};
		nvlist_lookup_nvpair(m_handle, key, &res);
		return {m_handle, res};
	}

	bool nv_list::erase(const char* key) {
		if(m_handle == nullptr) return false;
		return nvlist_remove_all(m_handle, key) == 0;
	}

	void nv_list::add_boolean(const char* key) {
		ensure_allocated();
		auto res = nvlist_add_boolean(m_handle, key);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_boolean_value(const char* key, bool val) {
		ensure_allocated();
		auto res = nvlist_add_boolean_value(m_handle, key, val ? B_TRUE : B_FALSE);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_byte(const char* key, uchar_t val) {
		ensure_allocated();
		auto res = nvlist_add_byte(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int8(const char* key, int8_t val) {
		ensure_allocated();
		auto res = nvlist_add_int8(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint8(const char* key, uint8_t val) {
		ensure_allocated();
		auto res = nvlist_add_uint8(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int16(const char* key, int16_t val) {
		ensure_allocated();
		auto res = nvlist_add_int16(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint16(const char* key, uint16_t val) {
		ensure_allocated();
		auto res = nvlist_add_uint16(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int32(const char* key, int32_t val) {
		ensure_allocated();
		auto res = nvlist_add_int32(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint32(const char* key, uint32_t val) {
		ensure_allocated();
		auto res = nvlist_add_uint32(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int64(const char* key, int64_t val) {
		ensure_allocated();
		auto res = nvlist_add_int64(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint64(const char* key, uint64_t val) {
		ensure_allocated();
		auto res = nvlist_add_uint64(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_string(const char* key, const char* val) {
		ensure_allocated();
		auto res = nvlist_add_string(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_nvlist(const char* key, const nv_list& val) {
		ensure_allocated();
		if (val.m_handle == nullptr) {
			nv_list empty;
			empty.ensure_allocated();
			auto res = nvlist_add_nvlist(m_handle, key, empty.raw());
			if (res != 0) throw std::system_error(res, std::system_category());
		} else {
			auto res = nvlist_add_nvlist(m_handle, key, val.raw());
			if (res != 0) throw std::system_error(res, std::system_category());
		}
	}

	void nv_list::add_boolean_array(const char* key, const bool* val, size_t len) {
		ensure_allocated();
		std::vector<boolean_t> temp;
		temp.resize(len);
		for (size_t i = 0; i < len; i++)
			temp[i] = val[i] ? B_TRUE : B_FALSE;
		auto res = nvlist_add_boolean_array(m_handle, key, temp.data(), temp.size());
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_byte_array(const char* key, const uchar_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_byte_array(m_handle, key, const_cast<uchar_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int8_array(const char* key, const int8_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_int8_array(m_handle, key, const_cast<int8_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint8_array(const char* key, const uint8_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_uint8_array(m_handle, key, const_cast<uint8_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int16_array(const char* key, const int16_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_int16_array(m_handle, key, const_cast<int16_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint16_array(const char* key, const uint16_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_uint16_array(m_handle, key, const_cast<uint16_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int32_array(const char* key, const int32_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_int32_array(m_handle, key, const_cast<int32_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint32_array(const char* key, const uint32_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_uint32_array(m_handle, key, const_cast<uint32_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_int64_array(const char* key, const int64_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_int64_array(m_handle, key, const_cast<int64_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_uint64_array(const char* key, const uint64_t* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_uint64_array(m_handle, key, const_cast<uint64_t*>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_string_array(const char* key, const char* const* val, size_t len) {
		ensure_allocated();
		auto res = nvlist_add_string_array(m_handle, key, const_cast<char *const *>(val), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_nvlist_array(const char* key, const nv_list* val, size_t len) {
		ensure_allocated();
		std::vector<nvlist_t*> temp;
		temp.resize(len);
		for (size_t i = 0; i < len; i++) {
			temp[i] = val[i].raw();
		}
		auto res = nvlist_add_nvlist_array(m_handle, key, temp.data(), len);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	void nv_list::add_hrtime(const char* key, hrtime_t val) {
		ensure_allocated();
		auto res = nvlist_add_hrtime(m_handle, key, val);
		if (res != 0) throw std::system_error(res, std::system_category());
	}

	namespace {
		static std::string escape_string_json(const std::string& in) {
			constexpr const char* hextable = "0123456789ABCDEF";
			std::string res;
			res.reserve(in.size() * 1.10); // Rough guess of max 10% size increase
			for (auto e : in) {
				if (e == '\\')
					res += "\\\\";
				else if (e == '"')
					res += "\\\"";
				else if (e < 0x020) {
					res += "\\u00";
					res += hextable[static_cast<uint8_t>(e >> 4)];
					res += hextable[e & 0xf];
				} else
					res += e;
			}
			return res;
		}

		using std::to_string;
		std::string to_string(const std::string& s) { return std::string{s}; }
		std::string to_string(bool b) { return b ? "true" : "false"; }

		template<typename T>
		static std::string array_to_string(const std::vector<T>& arr, bool string_escape = false) {
			std::string res = "[ ";
			bool first = true;
			for (auto e : arr) {
				if (!first) res += ", ";
				first = false;
				if (string_escape)
					res += "\"" + escape_string_json(to_string(e)) + "\"";
				else
					res += to_string(e);
			}
			res += " ]";
			return res;
		}

		static void to_json_recursive(const nv_list& list, std::string indent, std::string& res, bool with_types) {
			res += "{";
			bool first = true;
			for (const auto& e : list) {
				if (!first) res += ",";
				first = false;
				res += "\n";
				res += indent + "\t\"";
				res += e.key();
				res += "\": ";
				if (with_types) res += "<" + std::string(nv_type_name(e.type())) + "> ";
				switch (e.type()) {
				case nv_type::boolean: res += to_string(e.as_boolean()); break;
				case nv_type::byte: res += to_string(e.as_byte()); break;
				case nv_type::int16: res += to_string(e.as_int16()); break;
				case nv_type::uint16: res += to_string(e.as_uint16()); break;
				case nv_type::int32: res += to_string(e.as_int32()); break;
				case nv_type::uint32: res += to_string(e.as_uint32()); break;
				case nv_type::int64: res += to_string(e.as_int64()); break;
				case nv_type::uint64: res += to_string(e.as_uint64()); break;
				case nv_type::string: res += "\"" + escape_string_json(e.as_string()) + "\""; break;
				case nv_type::nvlist: to_json_recursive(e.as_nvlist(), indent + "\t", res, with_types); break;
				case nv_type::boolean_array: res += array_to_string(e.as_boolean_array()); break;
				case nv_type::byte_array: res += array_to_string(e.as_byte_array()); break;
				case nv_type::int16_array: res += array_to_string(e.as_int16_array()); break;
				case nv_type::uint16_array: res += array_to_string(e.as_uint16_array()); break;
				case nv_type::int32_array: res += array_to_string(e.as_int32_array()); break;
				case nv_type::uint32_array: res += array_to_string(e.as_uint32_array()); break;
				case nv_type::int64_array: res += array_to_string(e.as_int64_array()); break;
				case nv_type::uint64_array: res += array_to_string(e.as_uint64_array()); break;
				case nv_type::string_array: res += array_to_string(e.as_uint64_array(), true); break;
				case nv_type::nvlist_array: {
					res += "[ ";
					bool arrfirst = true;
					for (const auto& i : e.as_nvlist_array()) {
						if (!arrfirst) res += ",\n";
						arrfirst = false;
						to_json_recursive(i, indent + "\t", res, with_types);
					}
					res += " ]";
				} break;
				default: res += "null"; break;
				}
			}
			res += "\n" + indent + "}";
		}
	} // namespace

	std::string nv_list::to_json(bool with_types) const {
		std::string res;
		to_json_recursive(*this, "", res, with_types);
		return res;
	}

} // namespace zfspp