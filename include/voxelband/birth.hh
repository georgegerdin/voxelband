#ifndef birth_hh__
#define birth_hh__

struct Birth {
	std::string charname;
	int selected_class = 0;
	int selected_race = 0;
	bool male_gender = true, female_gender = false;
	int str, dex, agi, vit, inte, wis, hp, mana;
	
	std::optional<std::pair<std::string, std::function<void ()>>> messagebox;

	struct SelectGender {};
	struct SelectRace {};
	struct SelectClass {};
	struct StatRoller {};
	struct EnterName {};
	struct Summary {};

	using ActiveMenu = std::variant<
		SelectGender,
		SelectRace,
		SelectClass,
		StatRoller,
		EnterName,
		Summary
	>;

	ActiveMenu active = SelectGender();

	void reroll();

	bool frame();

	void save_character();

	template<typename Tlambda>
	void MessageBox(const char* text, Tlambda on_ok) {
		messagebox = std::make_pair(text, on_ok);
	}

	Birth();
};

#endif //birth_hh__
