// sq_imgui.cpp — SFML bridge for Dear ImGui. See sq_imgui.hpp for the why.

#include "sq_imgui.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

namespace sq {
namespace imgui {

bool Bridge::init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    // No settings file: this is a device with no home directory to write one
    // to, and an immediate-mode kit that quietly misplaces its state on disk
    // is worse than one that keeps it in memory.
    io.IniFilename = nullptr;

    const bool ok = ImGui_ImplOpenGL3_Init("#version 300 es");
    initialized_ = ok;
    return ok;
}

void Bridge::shutdown()
{
    if (!initialized_)
        return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

void Bridge::processEvent(const sf::Event& event)
{
    if (!initialized_)
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (const auto* resized = event.getIf<sf::Event::Resized>())
    {
        io.DisplaySize = ImVec2(static_cast<float>(resized->size.x),
                                static_cast<float>(resized->size.y));
        return;
    }

    if (const auto* began = event.getIf<sf::Event::TouchBegan>())
    {
        if (began->finger == 0)
        {
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(static_cast<float>(began->position.x),
                                static_cast<float>(began->position.y));
            io.AddMouseButtonEvent(0, true);
        }
        return;
    }

    if (const auto* moved = event.getIf<sf::Event::TouchMoved>())
    {
        if (moved->finger == 0)
            io.AddMousePosEvent(static_cast<float>(moved->position.x),
                                static_cast<float>(moved->position.y));
        return;
    }

    if (const auto* ended = event.getIf<sf::Event::TouchEnded>())
    {
        if (ended->finger == 0)
        {
            io.AddMousePosEvent(static_cast<float>(ended->position.x),
                                static_cast<float>(ended->position.y));
            io.AddMouseButtonEvent(0, false);
        }
        return;
    }
}

void Bridge::newFrame(const sf::Time& dt, sf::RenderTarget& target)
{
    if (!initialized_)
        return;

    ImGuiIO& io = ImGui::GetIO();

    const sf::Vector2u size = target.getSize();
    io.DisplaySize          = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
    io.DeltaTime            = dt.asSeconds();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void Bridge::render()
{
    if (!initialized_)
        return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace imgui
}  // namespace sq