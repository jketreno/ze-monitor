#include <ncurses.h>
#include <panel.h>
#include <cstring>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "device.h"
#include "temperature.h"
#include "process.h"
#include "engine.h"
#include "helpers.h"

// Color pairs
enum ColorPairs {
    COLOR_PAIR_DEFAULT = 1,
    COLOR_PAIR_HEADER,
    COLOR_PAIR_SELECTED,
    COLOR_PAIR_UTILIZATION_LOW,
    COLOR_PAIR_UTILIZATION_MED,
    COLOR_PAIR_UTILIZATION_HIGH,
    COLOR_PAIR_TEMP_NORMAL,
    COLOR_PAIR_TEMP_WARNING,
    COLOR_PAIR_TEMP_CRITICAL
};

// Application state
struct AppState {
    Device* device;
    int selected_row = 0;
    int scroll_offset = 0;
    bool running = true;
    int sort_column = 0;
    bool sort_ascending = false;
};

// Utility function to draw progress bar
void draw_progress_bar(WINDOW* win, int y, int x, int width, float percentage, 
                       bool with_border = true, bool use_color = true) {
    int bar_width = width - 7; // Account for percentage text
    int fill_width = static_cast<int>(percentage * bar_width / 100.0);
    
    if (with_border) {
        mvwaddch(win, y, x, '[');
        mvwaddch(win, y, x + bar_width + 1, ']');
    }
    
    // Draw the filled part
    if (use_color) {
        int color_pair = COLOR_PAIR_UTILIZATION_LOW;
        if (percentage > 75) {
            color_pair = COLOR_PAIR_UTILIZATION_HIGH;
        } else if (percentage > 40) {
            color_pair = COLOR_PAIR_UTILIZATION_MED;
        }
        wattron(win, COLOR_PAIR(color_pair));
        for (int i = 0; i < fill_width; i++) {
            mvwaddch(win, y, x + 1 + i, '|');
        }
        wattroff(win, COLOR_PAIR(color_pair));
    } else {
        for (int i = 0; i < fill_width; i++) {
            mvwaddch(win, y, x + 1 + i, '|');
        }
    }
    
    // Draw the empty part
    for (int i = fill_width; i < bar_width; i++) {
        mvwaddch(win, y, x + 1 + i, ' ');
    }
    
    // Display percentage
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << percentage << "%";
    mvwprintw(win, y, x + bar_width + 3, "%s", ss.str().c_str());
}

// Initialize colors
void init_colors() {
    start_color();
    init_pair(COLOR_PAIR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PAIR_HEADER, COLOR_BLACK, COLOR_CYAN);
    init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_PAIR_UTILIZATION_LOW, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_UTILIZATION_MED, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_UTILIZATION_HIGH, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_TEMP_NORMAL, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_TEMP_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_TEMP_CRITICAL, COLOR_RED, COLOR_BLACK);
}

// Draw headers
void draw_header(WINDOW* win, int width) {
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    mvwhline(win, 0, 0, ' ', width);
    mvwprintw(win, 0, 2, "ze-monitor - Press 'q' to quit, 'h' for help");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    wrefresh(win);
}

// Draw engine utilization panel
void draw_engines_panel(WINDOW* win, AppState& state) {
    int y = 1;
    int width, height;
    getmaxyx(win, height, width);
    
    mvwprintw(win, y++, 1, "Engine Utilization:");
    
    for (size_t i = 0; i < state.device->getEngineCount() && y < height - 1; i++) {
        float utilization = state.device->getEngine(i)->getEngineUtilization();
        
        // Replace with actual calls to your Engine class
        std::string name = "Engine " + std::to_string(i); // engine->getName();
        
        mvwprintw(win, y, 1, "%s:", name.c_str());
        draw_progress_bar(win, y, name.length() + 3, width - name.length() - 5, utilization);
        y++;
    }

    wrefresh(win);
}

// Draw temperature panel
void draw_temperature_panel(WINDOW* win, AppState& state) {
    int y = 1;
    int width, height;
    getmaxyx(win, height, width);
    
    mvwprintw(win, y++, 1, "Temperature Sensors:");
    state.device->updateTemperatures();
    for (size_t i = 0; i < state.device->getTemperatureCount() && y < height - 1; i++) {
        float temp = state.device->getTemperature(i);
        
        // Replace with actual calls to your TemperatureMonitor class
        std::string name = "Sensor " + std::to_string(i); // sensor->getName();
        
        int color_pair = COLOR_PAIR_TEMP_NORMAL;
        if (temp > 85) {
            color_pair = COLOR_PAIR_TEMP_CRITICAL;
        } else if (temp > 70) {
            color_pair = COLOR_PAIR_TEMP_WARNING;
        }
        
        mvwprintw(win, y, 1, "%s:", name.c_str());
        wattron(win, COLOR_PAIR(color_pair));
        mvwprintw(win, y, name.length() + 3, "%.1f°C", temp);
        wattroff(win, COLOR_PAIR(color_pair));
        y++;
    }

    wrefresh(win);
}

// Draw processes panel
void draw_processes_panel(WINDOW* win, AppState& state) {
    int width, height;
    getmaxyx(win, height, width);
    
    // Header
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    mvwhline(win, 1, 0, ' ', width);
    mvwprintw(win, 1, 1, "PID");
    mvwprintw(win, 1, 8, "Process");
    mvwprintw(win, 1, 35, "Engine");
    mvwprintw(win, 1, 50, "Utilization");
    mvwprintw(win, 1, 65, "Memory");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    
    state.device->updateProcesses();

    int visible_rows = height - 3;  // -2 for header and border, -1 for status
    int max_scroll = std::max(0, static_cast<int>(state.device->getProcessCount()) - visible_rows);
    state.scroll_offset = std::min(state.scroll_offset, max_scroll);
    
    std::vector<const ProcessInfo *> processList;
    for (uint32_t i = 0; i < state.device->getProcessCount(); ++i) {
        processList.emplace_back(state.device->getProcessInfo(i));
    }

    // Sort processes (replace with actual logic based on your Process class)
    std::sort(processList.begin(), processList.end(), 
              [&state](const ProcessInfo* a, const ProcessInfo* b) {
                  // Replace with actual comparisons based on your Process class
                  int result = 0;
                  switch (state.sort_column) {
                      case 0: // PID
                          result = a->getProcessState()->processId - b->getProcessState()->processId;
                          break;
                      case 1: // Name
                          result = a->getCommandLine().compare(b->getCommandLine());
                          break;
                      case 2: // Engine
                          result = 1; // a->getEngineName().compare(b->getEngineName());
                          break;
                      case 3: // Utilization
                          result = 1; // a->getUtilization() - b->getUtilization();
                          break;
                      case 4: // Memory
                          result = a->getProcessState()->memSize - b->getProcessState()->memSize;
                          break;
                  }
                  return state.sort_ascending ? result < 0 : result > 0;
              });
    
    // Draw processes
    for (int i = 0; i < visible_rows && i + state.scroll_offset < static_cast<int>(processList.size()); i++) {
        int y = i + 2;  // +2 for header
        int idx = i + state.scroll_offset;
        const ProcessInfo* process = processList[idx];
        
        // Highlight selected row
        if (idx == state.selected_row) {
            wattron(win, COLOR_PAIR(COLOR_PAIR_SELECTED));
            mvwhline(win, y, 0, ' ', width);
        }
        
        // Replace with actual calls to your Process class
        int pid = process->getProcessState()->processId;
        std::string name = process->getCommandLine();
        std::string engine = "Engines " + engine_flags_to_str(process->getProcessState()->engines);
        float utilization = 20.0f + (rand() % 80); // process->getUtilization();
        float memory = process->getProcessState()->memSize;
        
        mvwprintw(win, y, 1, "%d", pid);
        mvwprintw(win, y, 8, "%s", name.c_str());
        mvwprintw(win, y, 35, "%s", engine.c_str());
        
        // Draw utilization bar
        draw_progress_bar(win, y, 50, 14, utilization, false, false);
        
        // Memory
        mvwprintw(win, y, 65, "%.1f MB", memory);
        
        if (idx == state.selected_row) {
            wattroff(win, COLOR_PAIR(COLOR_PAIR_SELECTED));
        }
    }
    
    // Status line
    mvwprintw(win, height - 1, 1, "Processes: %zu | Scroll: %d/%d | Press F1-F5 to sort",
              (size_t)state.device->getProcessCount(), state.scroll_offset, max_scroll);

    wrefresh(win);
}

// Draw help panel
void draw_help_panel(WINDOW* win) {
    int y = 1;
    
    mvwprintw(win, y++, 2, "ze-monitor Help");
    y++;
    mvwprintw(win, y++, 2, "Navigation:");
    mvwprintw(win, y++, 4, "Up/Down - Select process");
    mvwprintw(win, y++, 4, "Page Up/Down - Scroll process list");
    y++;
    mvwprintw(win, y++, 2, "Sorting:");
    mvwprintw(win, y++, 4, "F1 - Sort by PID");
    mvwprintw(win, y++, 4, "F2 - Sort by Process name");
    mvwprintw(win, y++, 4, "F3 - Sort by Engine");
    mvwprintw(win, y++, 4, "F4 - Sort by Utilization");
    mvwprintw(win, y++, 4, "F5 - Sort by Memory");
    y++;
    mvwprintw(win, y++, 2, "Other:");
    mvwprintw(win, y++, 4, "h - Toggle help");
    mvwprintw(win, y++, 4, "q - Quit");
    
    mvwprintw(win, y + 2, 2, "Press any key to close help");
    wrefresh(win);
}

// Main application entry point
int ze_ncurses_main(Device *device, uint32_t interval, bool oneShot) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor
    timeout(interval); // Set getch timeout for non-blocking input (refresh rate)
    
    // Initialize colors
    if (has_colors()) {
        init_colors();
    }
    
    // Get screen dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Create main window with border
    WINDOW* main_win = newwin(max_y, max_x, 0, 0);
    box(main_win, 0, 0);
    
    // Calculate panel heights
    int engine_height = 6;
    int temp_height = 6;
    int process_height = max_y - engine_height - temp_height;
    
    // Create subwindows
    WINDOW* header_win = derwin(main_win, 1, max_x - 2, 1, 1);
    WINDOW* engine_win = derwin(main_win, engine_height, max_x - 2, 2, 1);
    WINDOW* temp_win = derwin(main_win, temp_height, max_x - 2, 2 + engine_height, 1);
    WINDOW* process_win = derwin(main_win, process_height, max_x - 2, 2 + engine_height + temp_height, 1);
    
    // Create help window (initially hidden)
    WINDOW* help_win = newwin(20, 50, (max_y - 20) / 2, (max_x - 50) / 2);
    box(help_win, 0, 0);
    PANEL* help_panel = new_panel(help_win);
    hide_panel(help_panel);
    
    // Set up application state with dummy data
    AppState state;
    
    // Replace with actual instances of your classes
    state.device = device; // new Device();
    
    
    bool show_help = false;
    
    // Main event loop
    while (state.running) {
        // Clear windows
        werase(header_win);
        werase(engine_win);
        werase(temp_win);
        werase(process_win);
        
        // Draw border
        box(main_win, 0, 0);
        
        // Draw content
        draw_header(header_win, max_x - 2);
        draw_engines_panel(engine_win, state);
        draw_temperature_panel(temp_win, state);
        draw_processes_panel(process_win, state);
        
        // Draw help if visible
        if (show_help) {
            draw_help_panel(help_win);
            show_panel(help_panel);
        } else {
            hide_panel(help_panel);
        }
        
        // Update panels and refresh
        update_panels();
        doupdate();
        wrefresh(main_win);

        // Handle input
        int ch = getch();
        if (ch != ERR) {  // If a key was pressed
            if (show_help) {
                show_help = false;
                continue;
            }
            
            switch (ch) {
                case 'q':
                    state.running = false;
                    break;
                case 'h':
                    show_help = true;
                    break;
                case KEY_UP:
                    if (state.selected_row > 0) {
                        state.selected_row--;
                        // Adjust scroll if needed
                        if (state.selected_row < state.scroll_offset) {
                            state.scroll_offset = state.selected_row;
                        }
                    }
                    break;
                case KEY_DOWN:
                    if (state.selected_row < static_cast<int>(state.device->getProcessCount()) - 1) {
                        state.selected_row++;
                        // Adjust scroll if needed
                        int visible_rows = process_height - 3;
                        if (state.selected_row >= state.scroll_offset + visible_rows) {
                            state.scroll_offset = state.selected_row - visible_rows + 1;
                        }
                    }
                    break;
                case KEY_PPAGE:  // Page Up
                    state.scroll_offset = std::max(0, state.scroll_offset - (process_height - 3));
                    break;
                case KEY_NPAGE:  // Page Down
                    {
                        int visible_rows = process_height - 3;
                        int max_scroll = std::max(0, static_cast<int>(state.device->getProcessCount()) - visible_rows);
                        state.scroll_offset = std::min(max_scroll, state.scroll_offset + visible_rows);
                    }
                    break;
                case KEY_F(1):
                case KEY_F(2):
                case KEY_F(3):
                case KEY_F(4):
                case KEY_F(5):
                    {
                        int new_sort_column = ch - KEY_F(1);
                        if (new_sort_column == state.sort_column) {
                            state.sort_ascending = !state.sort_ascending;
                        } else {
                            state.sort_column = new_sort_column;
                            state.sort_ascending = true;
                        }
                    }
                    break;
            }
        }       

        if (oneShot) {
            break;
        }
    }
    
    // Save screen content before exiting
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    std::vector<std::string> screen_content(rows);
    for (int i = 0; i < rows; ++i)
    {
        std::vector<char> line(cols + 1, '\0');
        move(i, 0);
        winnstr(process_win, line.data(), cols); // Capture line from ncurses screen
        screen_content[i] = std::string(line.data());
    }

    
    // Clean up
    del_panel(help_panel);
    delwin(help_win);
    delwin(process_win);
    delwin(temp_win);
    delwin(engine_win);
    delwin(header_win);
    delwin(main_win);
    endwin();

    // Restore screen contents
    for (const auto &line : screen_content)
    {
        std::cout << line << std::endl;
    }
    
    return 0;
}
