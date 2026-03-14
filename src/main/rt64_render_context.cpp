// RT64 Render Context for San Francisco Rush
// Based on Extreme-G's proven RT64 integration

#include "hle/rt64_application.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "ultramodern/renderer_context.hpp"
#include "librecomp/game.hpp"

static uint8_t s_dmem[0x1000];
static uint8_t s_imem[0x1000];

static uint32_t s_dpc_start = 0;
static uint32_t s_dpc_end = 0;
static uint32_t s_dpc_current = 0;
static uint32_t s_dpc_status = 0;
static uint32_t s_dpc_clock = 0;
static uint32_t s_dpc_bufbusy = 0;
static uint32_t s_dpc_pipebusy = 0;
static uint32_t s_dpc_tmem = 0;
static uint32_t s_mi_intr = 0;

static void dummy_check_interrupts() {
}

class SFRushRenderContext : public ultramodern::renderer::RendererContext {
public:
    SFRushRenderContext(uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode)
        : rdram_(rdram), window_handle_(window_handle), developer_mode_(developer_mode)
    {
        setup_result = ultramodern::renderer::SetupResult::Success;
        chosen_api = ultramodern::renderer::GraphicsApi::D3D12;

        vi_regs_ = ultramodern::renderer::get_vi_regs();

        RT64::Application::Core core{};
        core.window = window_handle.window;
        core.RDRAM = rdram;
        core.DMEM = s_dmem;
        core.IMEM = s_imem;

        auto rom = recomp::get_rom();
        if (rom.size() >= 0x40) {
            rom_header_.assign(rom.begin(), rom.begin() + 0x40);
            core.HEADER = rom_header_.data();
        } else {
            rom_header_.resize(0x40, 0);
            core.HEADER = rom_header_.data();
        }

        core.MI_INTR_REG = &s_mi_intr;
        core.DPC_START_REG = &s_dpc_start;
        core.DPC_END_REG = &s_dpc_end;
        core.DPC_CURRENT_REG = &s_dpc_current;
        core.DPC_STATUS_REG = &s_dpc_status;
        core.DPC_CLOCK_REG = &s_dpc_clock;
        core.DPC_BUFBUSY_REG = &s_dpc_bufbusy;
        core.DPC_PIPEBUSY_REG = &s_dpc_pipebusy;
        core.DPC_TMEM_REG = &s_dpc_tmem;

        core.VI_STATUS_REG = &vi_regs_->VI_STATUS_REG;
        core.VI_ORIGIN_REG = &vi_regs_->VI_ORIGIN_REG;
        core.VI_WIDTH_REG = &vi_regs_->VI_WIDTH_REG;
        core.VI_INTR_REG = &vi_regs_->VI_INTR_REG;
        core.VI_V_CURRENT_LINE_REG = &vi_regs_->VI_V_CURRENT_LINE_REG;
        core.VI_TIMING_REG = &vi_regs_->VI_TIMING_REG;
        core.VI_V_SYNC_REG = &vi_regs_->VI_V_SYNC_REG;
        core.VI_H_SYNC_REG = &vi_regs_->VI_H_SYNC_REG;
        core.VI_LEAP_REG = &vi_regs_->VI_LEAP_REG;
        core.VI_H_START_REG = &vi_regs_->VI_H_START_REG;
        core.VI_V_START_REG = &vi_regs_->VI_V_START_REG;
        core.VI_V_BURST_REG = &vi_regs_->VI_V_BURST_REG;
        core.VI_X_SCALE_REG = &vi_regs_->VI_X_SCALE_REG;
        core.VI_Y_SCALE_REG = &vi_regs_->VI_Y_SCALE_REG;

        core.checkInterrupts = dummy_check_interrupts;

        RT64::ApplicationConfiguration app_config;
        app_config.appId = "sfrush";
        app_config.detectDataPath = true;
        app_config.useConfigurationFile = true;

        fprintf(stderr, "[SFRush] Creating RT64 application...\n");
        app_ = std::make_unique<RT64::Application>(core, app_config);

        auto result = app_->setup(window_handle.thread_id);

        switch (result) {
        case RT64::Application::SetupResult::Success:
            fprintf(stderr, "[SFRush] RT64 initialized successfully\n");
            is_valid_ = true;
            break;
        case RT64::Application::SetupResult::DynamicLibrariesNotFound:
            fprintf(stderr, "[SFRush] RT64: Dynamic libraries not found\n");
            setup_result = ultramodern::renderer::SetupResult::DynamicLibrariesNotFound;
            break;
        case RT64::Application::SetupResult::InvalidGraphicsAPI:
            fprintf(stderr, "[SFRush] RT64: Invalid graphics API\n");
            setup_result = ultramodern::renderer::SetupResult::InvalidGraphicsAPI;
            break;
        case RT64::Application::SetupResult::GraphicsAPINotFound:
            fprintf(stderr, "[SFRush] RT64: Graphics API not found\n");
            setup_result = ultramodern::renderer::SetupResult::GraphicsAPINotFound;
            break;
        case RT64::Application::SetupResult::GraphicsDeviceNotFound:
            fprintf(stderr, "[SFRush] RT64: Graphics device not found\n");
            setup_result = ultramodern::renderer::SetupResult::GraphicsDeviceNotFound;
            break;
        }

        if (!is_valid_) {
            fprintf(stderr, "[SFRush] RT64 setup failed\n");
            app_.reset();
        }
    }

    ~SFRushRenderContext() override {
        if (app_) {
            fprintf(stderr, "[SFRush] Destroying RT64 application\n");
            app_.reset();
        }
    }

    bool valid() override { return is_valid_; }

    bool update_config(const ultramodern::renderer::GraphicsConfig& old_config,
                       const ultramodern::renderer::GraphicsConfig& new_config) override {
        return true;
    }

    void enable_instant_present() override {}

    void send_dl(const OSTask* task) override {
        if (!app_ || !is_valid_) return;

        dl_count_++;
        uint32_t dl_address = task->t.data_ptr & 0x1FFFFFFF;
        if (dl_count_ <= 5 || (dl_count_ % 60 == 0)) {
            fprintf(stderr, "[SFRush] Display list #%u at 0x%08X\n", dl_count_, dl_address);
        }
        app_->processDisplayLists(rdram_, dl_address, 0, true);
    }

    void update_screen() override {
        if (!app_ || !is_valid_) return;

        screen_count_++;
        if (screen_count_ <= 5 || (screen_count_ % 60 == 0)) {
            fprintf(stderr, "[SFRush] update_screen #%u\n", screen_count_);
        }
        app_->updateScreen();
    }

    void shutdown() override {
        fprintf(stderr, "[SFRush] Render context shutdown\n");
        if (app_) app_.reset();
        is_valid_ = false;
    }

    uint32_t get_display_framerate() const override { return 60; }
    float get_resolution_scale() const override { return 1.0f; }

private:
    uint8_t* rdram_;
    ultramodern::renderer::WindowHandle window_handle_;
    bool developer_mode_;
    bool is_valid_ = false;
    ultramodern::renderer::ViRegs* vi_regs_ = nullptr;
    std::unique_ptr<RT64::Application> app_;
    std::vector<uint8_t> rom_header_;
    uint32_t dl_count_ = 0;
    uint32_t screen_count_ = 0;
};

std::unique_ptr<ultramodern::renderer::RendererContext> create_sfrush_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    return std::make_unique<SFRushRenderContext>(rdram, window_handle, developer_mode);
}
