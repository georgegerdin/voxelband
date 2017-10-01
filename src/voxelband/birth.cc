#include <variant>
#include <string>

#include <algorithm>
#include <optional>
#include <functional>
#include <boost/hana/functional/overload.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include "imgui/imgui.h"
#include "birth.hh"
#include "namegen.h"
#include <nlohmann/json.hpp>

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

namespace fs = boost::filesystem;

const char* races[] = {
	"Human",
	"Dwarf",
	"Elf",
	"Half-Elf"
};

const char* classes[] = {
	"Fighter",
	"Rogue",
	"Paladin",
	"Mage",
	"Cleric"
};

const char* genders[] = {
	"Male",
	"Female"
};

const int base_hp[] = {
	30,
	20,
	25,
	15,
	15
};

const int base_mana[] = {
	10,
	20,
	20,
	30,
	30
};

using std::to_string;

std::string to_string(const char* s) {
	return std::string(s);
}

template<typename T>
void UniqueButton(T text, const char* id) {
	std::string str = to_string(text);
	str += "##";
	str += id;
	ImGui::Button(str.c_str());
}

void Birth::reroll() {
	str = std::max(rand() % 18, 3);
	dex = std::max(rand() % 18, 3);
	agi = std::max(rand() % 18, 3);
	vit = std::max(rand() % 18, 3);
	inte = std::max(rand() % 18, 3);
	wis = std::max(rand() % 18, 3);

	hp = base_hp[selected_class] + (rand() % 30);
	mana = base_mana[selected_class] + (rand() % 30);

}

void Birth::save_character() {
	nlohmann::json j;

	{
		//Open character info file
		std::ifstream chars("characters.dat");
		if(chars.is_open())
			chars >> j;

		//Generate a world directory path from hash of character name
		auto base_path = fs::path("world");
		boost::hash<std::string> hash_func;
		auto hash = hash_func(charname.c_str());

		//Make sure world directory path is unique in case
		//Multiple characters havev same name or hash
		fs::path char_path;
		do {
			char_path = base_path / to_string(hash);
			++hash;
		} while (fs::exists(char_path));
		fs::create_directories(char_path);

		//Save char info in json format
		nlohmann::json char_entry;
		char_entry["name"] = charname.c_str();
		char_entry["dir"] = char_path.generic_string();
		j.emplace_back(char_entry);
	}

	//Write character data to file;
	std::ofstream o("characters.dat");
	o << std::setw(4) << j << std::endl;
}

Birth::Birth() {
	NameGen::Generator generator("si");
	auto generated_name = generator.toString();
	charname = generated_name;
	charname[0] = toupper(charname[0]);
	charname.resize(128);
}

bool Birth::frame() {
	bool giving_birth = true;
	std::visit(boost::hana::overload(
		[&](SelectGender& ) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 100.f), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Select gender of your character");
			if (ImGui::RadioButton("Male", male_gender)) {
				male_gender = true;
				female_gender = false;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Female", female_gender)) {
				female_gender = true;
				male_gender = false;
			}
			if (ImGui::Button("Cancel"))
				giving_birth = false;
			ImGui::SameLine();
			if (ImGui::Button("Next"))
				active = SelectRace{};
			ImGui::End();
		},
		[&](SelectRace& ) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 170.f), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Select race of your character");

			int old_race = selected_race;
			ImGui::ListBox("", &selected_race, races, IM_ARRAYSIZE(races), -1);
			if (old_race != selected_race)
				reroll();

			if (ImGui::Button("Back"))
				active = SelectGender{};
			ImGui::SameLine();
			if (ImGui::Button("Next"))
				active = SelectClass{};
			ImGui::End();
		},
		[&](SelectClass&) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 180.f), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Select class of your character");
			
			int old_class = selected_class;
			ImGui::ListBox("", &selected_class, classes, IM_ARRAYSIZE(classes), -1);
			if (old_class != selected_class)
				reroll();

			if (ImGui::Button("Back"))
				active = SelectRace{};
			ImGui::SameLine();
			if (ImGui::Button("Next")) {
				reroll();
				active = StatRoller{};
			}

			ImGui::End();
		},
		[&](StatRoller&) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 400.f), ImGuiSetCond_FirstUseEver);

			ImGui::Begin("Stat roller");
			
			if (ImGui::Button("Re-roll stats"))
				reroll();

			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("BaseStats", ImVec2(0, 150), true);
			ImGui::Columns(2);
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Strength:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Dexterity:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Agility:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Vitality:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Wisdom:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Intelligence:");
			ImGui::NextColumn();
			UniqueButton(str, "1");
			UniqueButton(dex, "2");
			UniqueButton(agi, "3");
			UniqueButton(vit, "4");
			UniqueButton(wis, "5");
			UniqueButton(inte, "6");
			ImGui::EndChild();
			ImGui::BeginChild("VitalStats", ImVec2(0, 100), true);
			ImGui::Columns(2);
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Max HP:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Max Mana:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Level:");
			ImGui::NextColumn();
			UniqueButton(hp, "7");
			UniqueButton(mana, "8");
			UniqueButton("1", "9");
			ImGui::EndChild();
			ImGui::PopStyleVar();
			
			if (ImGui::Button("Back"))
				active = SelectClass{};
			ImGui::SameLine();
			if (ImGui::Button("Next"))
				active = EnterName{};
			ImGui::End();
		},
		[&](EnterName&) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 120.f), ImGuiSetCond_FirstUseEver);

			ImGui::Begin("Enter character name: ");
			ImGui::InputText("", charname.data(), charname.capacity());
			ImGui::SameLine();
			if (ImGui::Button("Generate")) {
				NameGen::Generator generator("si");
				auto generated_name = generator.toString();
				charname = generated_name;
				charname[0] = toupper(charname[0]);
				charname.resize(128);
			}
			if (ImGui::Button("Back"))
				active = StatRoller{};
			ImGui::SameLine();
			if (ImGui::Button("Finish")) {
				active = Summary{};
			}
			ImGui::End();
		},
		[&](Summary&) {
			ImGui::SetNextWindowSize(ImVec2(250.f, 420.f), ImGuiSetCond_FirstUseEver);

			ImGui::Begin("Character summary");
			ImGui::Text("Name: ");
			ImGui::SameLine();
			ImGui::Text(charname.c_str());
			ImGui::Text("Gender: ");
			ImGui::SameLine();
			ImGui::Text(genders[static_cast<int>(female_gender)]);
			ImGui::Text("Race: ");
			ImGui::SameLine();
			ImGui::Text(races[selected_race]);
			ImGui::Text("Class: ");
			ImGui::SameLine();
			ImGui::Text(races[selected_class]);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
			ImGui::BeginChild("BaseStats", ImVec2(0, 150), true);
			ImGui::Columns(2);
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Strength:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Dexterity:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Agility:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Vitality:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Wisdom:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Intelligence:");
			ImGui::NextColumn();
			UniqueButton(str, "1");
			UniqueButton(dex, "2");
			UniqueButton(agi, "3");
			UniqueButton(vit, "4");
			UniqueButton(wis, "5");
			UniqueButton(inte, "6");
			ImGui::EndChild();
			ImGui::BeginChild("VitalStats", ImVec2(0, 100), true);
			ImGui::Columns(2);
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Max HP:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Max Mana:");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Level:");
			ImGui::NextColumn();
			UniqueButton(hp, "7");
			UniqueButton(mana, "8");
			UniqueButton("1", "9");
			ImGui::EndChild();
			ImGui::PopStyleVar();

			if (ImGui::Button("Back"))
				active = StatRoller{};
			ImGui::SameLine();
			if (ImGui::Button("Accept")) {
				save_character();
				MessageBox("Character created.", [&] {giving_birth = false; });
			}
			ImGui::End();
		}
		), active);

	if (messagebox) {
		ImGui::OpenPopup("Message");
		if (ImGui::BeginPopupModal("Message")) {
			ImGui::Text(messagebox->first.c_str());
			if (ImGui::Button("OK")) {
				messagebox->second();
			}
			ImGui::EndPopup();
		}
	}
	return giving_birth;
}