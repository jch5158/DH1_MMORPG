#pragma once
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/table.h>
#include <sqlpp11/column.h>

namespace db
{
	namespace player_character_
	{
		struct CharacterId
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "character_id";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T characterId;
					T& operator()() { return characterId; }
					const T& operator()() const { return characterId; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::bigint, sqlpp::tag::must_not_update>;
		};

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
			using _traits = sqlpp::make_traits<sqlpp::bigint>;
		};

		struct CharacterName
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "character_name";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T characterName;
					T& operator()() { return characterName; }
					const T& operator()() const { return characterName; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::varchar>;
		};

		struct Level
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "level";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T level;
					T& operator()() { return level; }
					const T& operator()() const { return level; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::integer>;
		};

		struct Experience
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "experience";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T experience;
					T& operator()() { return experience; }
					const T& operator()() const { return experience; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::bigint>;
		};

		struct CurrentHp
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "current_hp";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T currentHp;
					T& operator()() { return currentHp; }
					const T& operator()() const { return currentHp; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct MaxHp
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "max_hp";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T maxHp;
					T& operator()() { return maxHp; }
					const T& operator()() const { return maxHp; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct PositionX
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "position_x";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T positionX;
					T& operator()() { return positionX; }
					const T& operator()() const { return positionX; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct PositionY
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "position_y";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T positionY;
					T& operator()() { return positionY; }
					const T& operator()() const { return positionY; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct PositionZ
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "position_z";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T positionZ;
					T& operator()() { return positionZ; }
					const T& operator()() const { return positionZ; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct RotationYaw
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "rotation_yaw";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T rotationYaw;
					T& operator()() { return rotationYaw; }
					const T& operator()() const { return rotationYaw; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::floating_point>;
		};

		struct WorldId
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "world_id";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T worldId;
					T& operator()() { return worldId; }
					const T& operator()() const { return worldId; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::integer>;
		};

		struct CreatedAt
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "created_at";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T createdAt;
					T& operator()() { return createdAt; }
					const T& operator()() const { return createdAt; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
		};

		struct LastPlayedAt
		{
			struct _alias_t
			{
				static constexpr const char _literal[] = "last_played_at";
				using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

				template<typename T>
				struct _member_t
				{
					T lastPlayedAt;
					T& operator()() { return lastPlayedAt; }
					const T& operator()() const { return lastPlayedAt; }
				};
			};
			using _traits = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
		};
	}

	struct PlayerCharacter : sqlpp::table_t<PlayerCharacter,
		player_character_::CharacterId,
		player_character_::AccountId,
		player_character_::CharacterName,
		player_character_::Level,
		player_character_::CurrentHp,
		player_character_::MaxHp,
		player_character_::Experience,
		player_character_::PositionX,
		player_character_::PositionY,
		player_character_::PositionZ,
		player_character_::RotationYaw,
		player_character_::WorldId,
		player_character_::CreatedAt,
		player_character_::LastPlayedAt>
	{
		struct _alias_t
		{
			static constexpr const char _literal[] = "player_character";
			using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;

			template<typename T>
			struct _member_t
			{
				T playerCharacter;
				T& operator()() { return playerCharacter; }
				const T& operator()() const { return playerCharacter; }
			};
		};
	};
}
