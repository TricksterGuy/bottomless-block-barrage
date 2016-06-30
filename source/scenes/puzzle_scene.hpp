#ifndef PUZZLE_SCENE
#define PUZZLE_SCENE

#include "game_scene.hpp"
#include <string>
#include <deque>
#include <windows/time_window.hpp>

class PuzzleScene : public GameScene
{
public:
    struct PuzzleConfig
    {
        std::string filename;
    };
    PuzzleScene(const PuzzleConfig& config);
    virtual void init_panel_table();
    virtual void init_menu();
    virtual void update_input();
    virtual void update_windows();
    bool is_gameover() const;
    virtual void draw_bottom();
private:
    std::deque<PanelTable> snapshots;
    PuzzleConfig puzzle_config;
    TimeWindow time_window;
};


#endif

