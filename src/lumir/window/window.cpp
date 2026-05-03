#include "Window.hpp"

namespace lumir {
	MainWindow::MainWindow(Syris::Window::CreateInfo info) : Window(info) {

	}
	bool MainWindow::on_event(Syris::Event* event){
		return false;
	}
	void MainWindow::on_update(const Syris::engine_time::Time& time){

	}
}
