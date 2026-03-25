#pragma once
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/table.h>
#include <sqlpp11/column.h>

namespace db
{
	namespace account_
	{
		struct AccountId
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "account_id";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T accountId;
					T& operator()() { return accountId; }
					const T& operator()() const { return accountId; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::bigint, sqlpp::tag::must_not_update>;
		};

		struct IsOnline
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "is_online";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T isOnline;
					T& operator()() { return isOnline; }
					const T& operator()() const { return isOnline; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::boolean>;
		};

		struct LastLogin
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "last_login";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T lastLogin;
					T& operator()() { return lastLogin; }
					const T& operator()() const { return lastLogin; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
		};

		struct LastLogout
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "last_logout";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T lastLogout;
					T& operator()() { return lastLogout; }
					const T& operator()() const { return lastLogout; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
		};
	}

	struct Account : sqlpp::table_t<Account,
		account_::AccountId,
		account_::IsOnline,
		account_::LastLogin,
		account_::LastLogout>
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "account";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

			template<typename T>
			struct _member_t
			{
				T account;
				T& operator()() { return account; }
				const T& operator()() const { return account; }
			};
		};
	};
}
