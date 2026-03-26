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
	}

	struct Account : sqlpp::table_t<Account,
		account_::AccountId,
		account_::LastLogin>
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
