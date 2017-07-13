#include "replay_simulation.hpp"

std::vector<Panel::Type> get_panels(const TraceState& initial)
{
    std::vector<Panel::Type> panels(72, Panel::Type::EMPTY);
    for (unsigned int i = 0; i < panels.size(); i++)
        panels[i] = (Panel::Type) (initial.panels[i] & 0xFF);
    return panels;
}

std::vector<Panel::Type> get_next_panels(const TraceManager& trace_manager)
{
    std::vector<Panel::Type> next;

    bool table_has_risen = false;
    const TraceState& state = trace_manager.GetInitialState();
    for (int j = 0; j < 6; j++)
        next.push_back((Panel::Type)(state.panels[72 + j] & 0xFF));


    for (uint32_t i = 0; i < trace_manager.GetFinalFrame(); i++)
    {
        const TraceState* state_ptr = trace_manager.GetState(i);
        if (state_ptr == nullptr) continue;

        const auto& state = *state_ptr;
        if (table_has_risen)
        {
            for (int j = 0; j < 6; j++)
                next.push_back((Panel::Type)(state.panels[72 + j] & 0xFF));
            table_has_risen = false;
        }
        if (state.panels[72] == 0xFF00FF)
            table_has_risen = true;
    }
    return next;
}

std::vector<Panel::Type> get_panels(const FrameState& initial)
{
    std::vector<Panel::Type> panels(72, Panel::Type::EMPTY);
    for (unsigned int i = 0; i < panels.size(); i++)
        panels[i] = (Panel::Type) (initial.panels[i] & 0xFF);
    return panels;
}

std::vector<Panel::Type> get_next_panels(const FrameStateManager& frame_manager)
{
    std::vector<Panel::Type> next;

    bool table_has_risen = false;
    const FrameState& state = frame_manager.GetInitialState();
    for (int j = 0; j < 6; j++)
        next.push_back((Panel::Type)(state.panels[72 + j] & 0xFF));


    for (uint32_t i = 0; i < frame_manager.GetFinalFrame(); i++)
    {
        const auto& state = frame_manager.GetState(i);
        if (table_has_risen)
        {
            for (int j = 0; j < 6; j++)
                next.push_back((Panel::Type)(state.panels[72 + j] & 0xFF));
            table_has_risen = false;
        }
        if (state.panels[72] == 0xFF00FF)
            table_has_risen = true;
    }
    return next;
}

ReplayPanelSource::ReplayPanelSource(const std::vector<Panel::Type>& _board, const std::vector<Panel::Type>& _next) : PanelSource(12, 6), table(_board), next(_next), index(0)
{
}

std::vector<Panel::Type> ReplayPanelSource::line()
{
    std::vector<Panel::Type> line(columns, Panel::Type::EMPTY);
    if (index > (int)next.size())
        return line;

    for (int i = 0; i < columns; i++)
    {
        line[i] = next[index + i];
    }

    index += columns;

    return line;
}

void ReplaySimulation::Step()
{
    DoStep();
    frame++;
}

void ReplaySimulation::Run(bool debug)
{
    while (!Finished())
    {
        if (debug) Print();
        Step();
    }
}

TraceReplaySimulation::TraceReplaySimulation(const TraceManager& trace_manager, const PanelSpeedSettings& settings) : table(nullptr), traces(trace_manager), last_input(0, 0)
{
    ReplayPanelSource* source = new ReplayPanelSource(get_panels(trace_manager.GetInitialState()), get_next_panels(trace_manager));
    PanelTable::Options opts;
    opts.source = source;
    opts.columns = 6;
    opts.rows = 12;
    opts.settings = settings;
    opts.type = PanelTable::Type::ENDLESS;
    table.reset(new PanelTable(opts));
}

void TraceReplaySimulation::DoStep()
{
    const auto& trace_ptr = traces.GetState(frame);
    const auto& input_ptr = traces.GetInput(frame);
    if (trace_ptr == nullptr || input_ptr == nullptr) return;

    const auto& trace = *trace_ptr;
    const auto& input = *input_ptr;

    if (input.button_a() && !last_input.button_a())
        table->swap(trace.selector_y, trace.selector_x);

    table->update(0x47);

    last_input = input;
}

void TraceReplaySimulation::Print()
{
    const auto& trace_ptr = traces.GetState(frame-1);
    const auto& input_ptr = traces.GetInput(frame-1);
    if (trace_ptr == nullptr || input_ptr == nullptr) return;

    const auto& trace = *trace_ptr;
    const auto& input = *input_ptr;

    printf("frame: %d\ninput: %x\naddress: %04x (%d %d) = %02x\nselector: (%d %d)\n", frame - 1, input.value(), trace.address, trace.y, trace.x, trace.value, trace.selector_y, trace.selector_x);
    for (unsigned int i = 0; i < 13; i++)
    {
        for (unsigned int j = 0; j < 6; j++)
        {
            printf("%08x ", trace.panels[j + i * 6]);
        }
        printf("\t\t");
        for (unsigned int j = 0; j < 6; j++)
        {
            Panel panel;
            if (i < 12)
                panel = table->get(i, j);
            else
                panel = table->get_next()[j];

            printf("%04x%02x%02x ", panel.get_countdown(), panel.get_state(), panel.get_value());
        }

        printf("\n");
    }
    printf("\n\n");
}

int calculate_timeout(int combo, int chain, int difficulty, bool in_danger)
{
    int combo_time;
    int chain_time;
    if (!in_danger)
    {
        combo_time = combo / 2 + difficulty - 2;
        chain_time = difficulty + chain + 1;
    }
    else
    {
        combo_time = (combo + 10) * difficulty / 5 + 1;
        chain_time = difficulty * (1 + chain) + 1;
    }
    if (combo <= 3)
        combo_time = 0;
    if (chain <= 1)
        chain_time = 0;

    return std::min(20, std::max(combo_time, chain_time));
}

FrameReplaySimulation::FrameReplaySimulation(const FrameStateManager& frame_manager, const PanelSpeedSettings& settings): table(nullptr), frames(frame_manager), last_input(0, 0), x(0), y(0)
{
    const FrameState& initial = frames.GetInitialState();
    ReplayPanelSource* source = new ReplayPanelSource(get_panels(initial), get_next_panels(frames));
    PanelTable::Options opts;
    opts.source = source;
    opts.columns = 6;
    opts.rows = 12;
    opts.settings = settings;
    opts.type = PanelTable::Type::ENDLESS;
    table.reset(new PanelTable(opts));

    x = initial.x;
    y = initial.y;
}

void FrameReplaySimulation::Print()
{
    const auto& state = frames.GetState(frame);
    printf("frame: %d\n", frame);
    printf("input: %x\n", state.input.value());
    printf("selector: %d %d vs %d %d\n", state.y, state.x, y, x);
    printf("score: %d\n", state.score);
    printf("level: %d\n", state.level);
    printf("next: %d\n", state.next);
    printf("combo: %d vs %d\n", state.combo, info.combo);
    printf("chain: %d vs %d\n", state.chain, info.chain);
    printf("timeout: %d vs %d\n", state.timeout, table->get_timeout());
    printf("rise counter: %x vs %x\n", state.rise_counter, table->get_rise_counter());
    printf("rise: %d vs %d\n", state.rise, table->get_rise());
    printf("speed: %x\n", state.rise_speed);

    for (unsigned int i = 0; i < 13; i++)
    {
        for (unsigned int j = 0; j < 6; j++)
        {
            printf("%08x ", state.panels[j + i * 6]);
        }
        printf("\t\t");
        for (unsigned int j = 0; j < 6; j++)
        {
            Panel panel;
            if (i < 12)
                panel = table->get(i, j);
            else
                panel = table->get_next()[j];

            printf("%04x%02x%02x ", panel.get_countdown(), panel.get_state(), panel.get_value());
        }

        printf("\n");
    }
    printf("\n\n");
}

void FrameReplaySimulation::DoStep()
{
    const auto& state = frames.GetState(frame);
    const auto& input = state.input;

    if (input.button_r() && !last_input.button_r())
        table->quick_rise();

    if (input.button_l() && !last_input.button_l())
        table->quick_rise();

    if (input.button_left() && !last_input.button_left())
        x = std::max(0, x - 1);

    if (input.button_right() && !last_input.button_right())
        x = std::min(4, x + 1);

    if (input.button_up() && !last_input.button_up())
        y = std::max(0, y - 1);

    if (input.button_down() && !last_input.button_down())
        y = std::min(11, y + 1);

    if (table->is_rised())
        y = std::max(0, y - 1);

    info = table->update(0x47);

    int timeout = calculate_timeout(info.combo, info.chain, 3, table->danger());
    table->freeze(timeout);

    if (input.button_a() && !last_input.button_a())
        table->swap(y, x);

    if (input.button_b() && !last_input.button_b())
        table->swap(y, x);


    last_input = input;
}

void FrameReplaySimulation::GetSelectorCoords(int& i, int& j) const
{
    i = y;
    j = x;
}
