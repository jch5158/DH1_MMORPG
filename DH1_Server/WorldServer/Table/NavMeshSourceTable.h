#pragma once
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/table.h>
#include <sqlpp11/column.h>

namespace db
{
	namespace world_navmesh_source_
	{
		struct Id
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "id";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T id; T& operator()() { return id; } const T& operator()() const { return id; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::bigint, sqlpp::tag::must_not_update>;
		};

		struct WorldServerId
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "world_server_id";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T worldServerId; T& operator()() { return worldServerId; } const T& operator()() const { return worldServerId; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::integer>;
		};

		struct MapCode
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "map_code";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T mapCode; T& operator()() { return mapCode; } const T& operator()() const { return mapCode; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::varchar>;
		};

		struct NavmeshPath
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "navmesh_path";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T navmeshPath; T& operator()() { return navmeshPath; } const T& operator()() const { return navmeshPath; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::varchar>;
		};

		struct NavmeshVersion
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "navmesh_version";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T navmeshVersion; T& operator()() { return navmeshVersion; } const T& operator()() const { return navmeshVersion; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::integer, sqlpp::tag::can_be_null>;
		};

		struct IsActive
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "is_active";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
				template<typename T> struct _member_t { T isActive; T& operator()() { return isActive; } const T& operator()() const { return isActive; } };
			};
			using _traits = sqlpp::make_traits<sqlpp::boolean>;
		};
	}

	struct WorldNavmeshSource : sqlpp::table_t<WorldNavmeshSource,
		world_navmesh_source_::Id,
		world_navmesh_source_::WorldServerId,
		world_navmesh_source_::MapCode,
		world_navmesh_source_::NavmeshPath,
		world_navmesh_source_::NavmeshVersion,
		world_navmesh_source_::IsActive>
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "world_navmesh_source";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
			template<typename T> struct _member_t { T worldNavmeshSource; T& operator()() { return worldNavmeshSource; } const T& operator()() const { return worldNavmeshSource; } };
		};
	};
}
