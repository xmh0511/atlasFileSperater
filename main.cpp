#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include "json.hpp"
#include <filesystem>
#include "code_parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <thread>
namespace fs = std::filesystem;
class Process {
public:
	Process() = default;
	Process(std::string const& mapsrc) :mapsrc_(mapsrc) {

	}
	Process& operator()(int x, int width, int y, int height) {
		auto img = cv::imread(mapsrc_.c_str(), -1);
		auto wpos = (x + width + 1) > img.cols ? img.cols : (x + width + 1);
		auto hpos = (y + height + 1) > img.rows ? img.rows : (y + height + 1);
		auto out = img(cv::Range(y, hpos), cv::Range(x, wpos));
		cv::imwrite(save_path_.c_str(), out);
		return *this;
	}
	void set_savepath(std::string const& src) {
		save_path_ = src;
	}
	std::string get_savepath() {
		return save_path_;
	}
private:
	std::string save_path_;
	std::string mapsrc_;
};
int main() {
	std::vector<std::unique_ptr<std::thread>> task;
	for (auto& iter : fs::directory_iterator("./resource")) {
		if (!fs::is_directory(iter)) {
			auto extension = xfinal::utf16_to_gbk(iter.path().filename().extension());
			auto fileName = xfinal::utf16_to_gbk(iter.path().filename());
			if (extension == ".atlas") {
				auto precedeName = fileName.substr(0, fileName.find("atlas")-1);
				auto config_src = xfinal::utf16_to_gbk(iter.path());
				auto map_src = std::string("./resource/") + precedeName + ".png";
				std::ifstream file(config_src.c_str());
				if (file.is_open()) {
					std::stringstream ss;
					ss << file.rdbuf();
					try {
						auto json = nlohmann::json::parse(ss.str());
						auto frams = json["frames"];
						auto save_path = std::string("./resource/") + precedeName+"/";
						if (!fs::exists(save_path)) {
							fs::create_directory(save_path);
						}
						task.emplace_back(std::make_unique<std::thread>([frams, save_path, map_src]() {
							Process pro{ map_src };
							for (auto jsonIter = frams.begin(); jsonIter != frams.end(); ++jsonIter) {
								auto filename = xfinal::utf8_to_gbk(jsonIter.key());
								std::cout << filename << "\n";
								pro.set_savepath(save_path + "/" + filename);
								auto value = jsonIter.value()["frame"];
								int x = value["x"].get<int>();
								int y = value["y"].get<int>();
								int w = value["w"].get<int>();
								int h = value["h"].get<int>();
								try {
									pro(x, w, y, h);
								}
								catch (std::exception& ec) {
									std::cout << x << "," << w << "," << y << "," << h << std::endl;
								}
							}
						}));
					}
					catch (std::exception& ec) {
						std::cout << ec.what() << std::endl;
					}
				}
			}
		}
	}
	//Process pro{ "./kongque/map.png" };
	//pro.set_savepath("./kongque/1.png");
	//pro(1503, 500, 1604, 400);
	for (auto& iter : task) {
		if (iter->joinable()) {
			iter->join();
		}
	}
}
