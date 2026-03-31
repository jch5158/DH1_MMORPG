using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace DbMigration.Migrations.Game
{
    /// <inheritdoc />
    public partial class AddPlayerCharacterHealthColumns : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<double>(
                name: "current_hp",
                table: "player_character",
                type: "double",
                nullable: false,
                defaultValue: 100.0);

            migrationBuilder.AddColumn<double>(
                name: "max_hp",
                table: "player_character",
                type: "double",
                nullable: false,
                defaultValue: 100.0);

            migrationBuilder.Sql(
                "UPDATE player_character SET max_hp = 100.0 + GREATEST(0, `level` - 1) * 20.0;");
            migrationBuilder.Sql(
                "UPDATE player_character SET current_hp = LEAST(current_hp, max_hp);");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "current_hp",
                table: "player_character");

            migrationBuilder.DropColumn(
                name: "max_hp",
                table: "player_character");
        }
    }
}
