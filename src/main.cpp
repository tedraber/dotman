#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

const char* home {std::getenv("HOME")};
// Location of .config
const fs::path config{fs::path(home) / ".config"};
// Location of dotman
const fs::path dotman{fs::path(home) / ".dotman"};
// Create dir for dotfile storage
const fs::path dfs{fs::path(dotman) / "dotfiles"};
// Tracking file
const fs::path tracking_file = dotman / "tracked.yaml";

// Function declarations
void makeFileStorage();
void listDots();
void genConfig();
void move_dots();
void save_tracked_dotfile(const std::string& config_name, const std::string& dotfile_name);
bool is_tracked(const std::string& name);
void list_tracked();
std::string get_active_config();
void save_current_config(const std::string& config_name);
void load_config(const std::string& config_name);
void list_available_configs();
void set_dot();

void makeFileStorage(){
    // Checks if dotman exists yet and creates it if not
    if (!fs::is_directory(fs::path(home) / ".dotman")) {
        std::cout << "Creating Storage Directory\n";  
        fs::create_directory(fs::path(home) / ".dotman");
        fs::create_directory(fs::path(dotman) / "dotfiles");
    }
}

void listDots(){
    for (const fs::directory_entry& entry : fs::directory_iterator(config)){
        int i{};
        if(entry.is_directory()){
            std::cout << entry.path().filename().string() << " ";
            if (i > 5) {
                std::cout << std::endl; 
                i = 0;
            }
            i++;
        }
    }
    std::cout << std::endl;
}

void genConfig(){
    std::cout << "Enter a name for the config: ";
    while(true){
        std::string dot;
        std::cin >> dot;
        std::cin.ignore(); // Clear newline
        
        fs::directory_entry dotName{dfs / dot};
        if (dotName.exists() && dotName.is_directory()){
            std::cout << "Already a directory, enter a new Name: ";  
        }
        else{
            std::cout << "Making directory\n";
            fs::create_directory(fs::path(dotName));
            
            // Initialize in tracking file
            YAML::Node tracker;
            if (fs::exists(tracking_file)) {
                tracker = YAML::LoadFile(tracking_file.string());
            }
            tracker["configs"][dot] = YAML::Node(YAML::NodeType::Map);
            
            std::ofstream fout(tracking_file);
            fout << tracker;
            
            std::cout << "Config '" << dot << "' created!\n";
            break;
        }
    }
}

void move_dots(){
    // First, ask which config to move into
    std::cout << "Which config should these dotfiles be added to?\n";
    list_available_configs();
    std::cout << "Enter config name (or 'new' to create one): ";
    
    std::string config_name;
    std::getline(std::cin, config_name);
    
    if (config_name == "new") {
        genConfig();
        std::cout << "Enter the name you just created: ";
        std::getline(std::cin, config_name);
    }
    
    fs::path config_dest = dfs / config_name;
    if (!fs::exists(config_dest)) {
        std::cout << "Config '" << config_name << "' doesn't exist.\n";
        return;
    }
    
    std::vector<fs::path> directories;
    std::string input;
    
    std::cout << "In ~/.config, enter directory paths separated by spaces EX: dir1 dir2 dir3 dir4\n";   
    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::string dir;

    while (iss >> dir) {
        directories.push_back(dir);
    }

    for(const auto& dir : directories) {
        fs::path source{config / dir};
        fs::path dest{config_dest / dir};

        if(fs::exists(source) && fs::is_directory(source)) {
            try{
                std::cout << "Found directory: " << dir << " moving now\n";
                fs::rename(source, dest);
                std::cout << "Moved: " << source << " -> " << dest << "\n";
                
                // Track it
                save_tracked_dotfile(config_name, dir);
                
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error moving " << source << ": " << e.what() << "\n";
            }
        } else {
            std::cout << "Not a valid directory: " << dir << '\n';
        }
    }
}

void save_tracked_dotfile(const std::string& config_name, const std::string& dotfile_name) {
    YAML::Node tracker;
    
    // Load existing file if it exists
    if (fs::exists(tracking_file)) {
        tracker = YAML::LoadFile(tracking_file.string());
    }
    
    // Add dotfile to config
    tracker["configs"][config_name][dotfile_name] = true;
    
    // Save back to file
    std::ofstream fout(tracking_file);
    fout << tracker;
}

bool is_tracked(const std::string& name) {
    if (!fs::exists(tracking_file)) return false;
    
    YAML::Node tracker = YAML::LoadFile(tracking_file.string());
    
    // Check all configs
    if (tracker["configs"]) {
        for (const auto& config : tracker["configs"]) {
            if (config.second[name]) {
                return true;
            }
        }
    }
    return false;
}

void list_tracked() {
    if (!fs::exists(tracking_file)) {
        std::cout << "No dotfiles tracked yet\n";
        return;
    }
    
    YAML::Node tracker = YAML::LoadFile(tracking_file.string());
    
    if (!tracker["configs"]) {
        std::cout << "No configs found\n";
        return;
    }
    
    std::cout << "Tracked configurations:\n";
    for (const auto& config : tracker["configs"]) {
        std::string config_name = config.first.as<std::string>();
        std::cout << "\n  [" << config_name << "]\n";
        
        for (const auto& dot : config.second) {
            std::cout << "    - " << dot.first.as<std::string>() << "\n";
        }
    }
}

std::string get_active_config() {
    if (!fs::exists(tracking_file)) return "";
    
    YAML::Node tracker = YAML::LoadFile(tracking_file.string());
    if (tracker["active_config"]) {
        return tracker["active_config"].as<std::string>();
    }
    return "";
}

void save_current_config(const std::string& config_name) {
    if (!fs::exists(tracking_file)) return;
    
    YAML::Node tracker = YAML::LoadFile(tracking_file.string());
    
    if (!tracker["configs"][config_name]) {
        std::cout << "Config not found in tracking file.\n";
        return;
    }
    
    std::cout << "Saving current config back to storage...\n";
    
    fs::path config_storage = dfs / config_name;
    fs::create_directories(config_storage);
    
    for (const auto& dot : tracker["configs"][config_name]) {
        std::string dot_name = dot.first.as<std::string>();
        fs::path source = config / dot_name;
        fs::path dest = config_storage / dot_name;
        
        if (fs::exists(source)) {
            try {
                fs::rename(source, dest);
                std::cout << "Saved: " << source << " -> " << dest << "\n";
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error saving " << source << ": " << e.what() << "\n";
            }
        }
    }
    
    // Clear active config
    tracker["active_config"] = "";
    std::ofstream fout(tracking_file);
    fout << tracker;
}

void load_config(const std::string& config_name) {
    fs::path config_path = dfs / config_name;
    
    if (!fs::exists(config_path)) {
        std::cout << "Config path doesn't exist.\n";
        return;
    }
    
    std::cout << "Loading config '" << config_name << "'...\n";
    
    for (const auto& entry : fs::directory_iterator(config_path)) {
        if (entry.is_directory()) {
            fs::path source = entry.path();
            fs::path dest = config / entry.path().filename();
            
            try {
                fs::rename(source, dest);
                std::cout << "Loaded: " << source << " -> " << dest << "\n";
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error loading " << source << ": " << e.what() << "\n";
            }
        }
    }
    
    // Update tracking file
    YAML::Node tracker;
    if (fs::exists(tracking_file)) {
        tracker = YAML::LoadFile(tracking_file.string());
    }
    tracker["active_config"] = config_name;
    
    std::ofstream fout(tracking_file);
    fout << tracker;
    
    std::cout << "Config '" << config_name << "' is now active!\n";
}

void list_available_configs() {
    if (!fs::exists(dfs)) {
        std::cout << "No configs found.\n";
        return;
    }
    
    bool found = false;
    for (const auto& entry : fs::directory_iterator(dfs)) {
        if (entry.is_directory()) {
            std::cout << "  - " << entry.path().filename().string() << "\n";
            found = true;
        }
    }
    
    if (!found) {
        std::cout << "No configs found.\n";
    }
}

void set_dot() {
    // Check if there's currently an active config
    std::string current_config = get_active_config();
    
    if (!current_config.empty()) {
        std::cout << "Currently active config: " << current_config << "\n";
        std::cout << "Switching will move your current .config files back to dotman storage.\n";
        std::cout << "Are you okay with this? (y/n): ";
        
        char response;
        std::cin >> response;
        std::cin.ignore(); // Clear newline from buffer
        
        if (response == 'y' || response == 'Y') {
            // Save current config back to storage
            save_current_config(current_config);
        } else {
            std::cout << "Would you like to create a new config instead? (y/n): ";
            std::cin >> response;
            std::cin.ignore();
            
            if (response == 'y' || response == 'Y') {
                genConfig();
                return;
            } else {
                std::cout << "Operation cancelled.\n";
                return;
            }
        }
    }
    
    // List available configs
    std::cout << "\nAvailable configurations:\n";
    list_available_configs();
    
    std::cout << "\nEnter config name to activate: ";
    std::string config_name;
    std::getline(std::cin, config_name);
    
    fs::path config_path = dfs / config_name;
    
    if (!fs::exists(config_path) || !fs::is_directory(config_path)) {
        std::cout << "Config '" << config_name << "' not found.\n";
        return;
    }
    
    // Load the selected config
    load_config(config_name);
}

int main() {
    makeFileStorage();
    
    while (true){
        std::cout << "=== Dotfile Manager ===\n";
        std::cout << "1. List configs\n";
        std::cout << "2. Create new config\n";
        std::cout << "3. Move dotfiles to config\n";
        std::cout << "4. Set active config\n";
        std::cout << "5. List tracked dotfiles\n";
        std::cout << "6. Exit\n";
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore();
    
        switch(choice) {
            case 1:
                list_available_configs();
                break;
            case 2:
                genConfig();
                break;
            case 3:
                move_dots();
                break;
            case 4:
                set_dot();
                break;
            case 5:
                list_tracked();
                break;
            case 6:
                std::cout << "Goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid choice\n";
            }
        }
    
    return 0;
}
