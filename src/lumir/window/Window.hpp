#pragma once

#include <syris/app/Window.hpp>
#include <syris/events/Event.hpp>
#include <syris/utils/EngineTime.hpp>

namespace lumir {
	class MainWindow : public Syris::Window {
	public:
		MainWindow(Syris::Window::CreateInfo info);
		~MainWindow() {};
        bool on_event(Syris::Event* event)override;
        void on_update(const Syris::engine_time::Time& time)override;
	};
}