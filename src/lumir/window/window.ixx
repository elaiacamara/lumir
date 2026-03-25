export module lumir.MainWindow;

import syris.window;
import syris.event;
import syris.engine_time;

namespace lumir {
	export class MainWindow : public Syris::Window {
	public:
		MainWindow(Syris::Window::CreateInfo info);
		~MainWindow() {};
        bool on_event(Syris::Event* event)override;
        void on_update(const Syris::engine_time::Time& time)override;
	};
}