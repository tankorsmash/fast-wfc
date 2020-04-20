#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include "time.h"

#include "overlapping_wfc.hpp"
#include "tiling_wfc.hpp"
#include "utils/array3D.hpp"
#include "wfc.hpp"

#include "../include/external/rapidxml.hpp"
#include "../include/rapidxml_utils.hpp"
//#include "rapidxml.hpp"
//#include "image.hpp"
//#include "rapidxml_utils.hpp"
//#include "utils.hpp"
#include <unordered_set>

#include "../../../src/include/utils/array2D.hpp"
#include "../include/color.hpp"
#include "../include/image.hpp"
#include "../include/utils.hpp"

using namespace rapidxml;

/**
 * Get a random seed.
 * This function use random_device on linux, but use the C rand function for
 * other targets. This is because, for instance, windows don't implement
 * random_device non-deterministically.
 */
int get_random_seed() {
#ifdef __linux__
    return random_device()();
#else
    return rand();
#endif
}

/**
 * Read the overlapping wfc problem from the xml node.
 */
void read_overlapping_instance(rapidxml::xml_node<> *node) {
    std::string name = rapidxml::get_attribute(node, "name");
    unsigned N = stoi(rapidxml::get_attribute(node, "N"));
    bool periodic_output =
        (rapidxml::get_attribute(node, "periodic", "False") == "True");
    bool periodic_input =
        (rapidxml::get_attribute(node, "periodicInput", "True") == "True");
    bool ground = (stoi(rapidxml::get_attribute(node, "ground", "0")) != 0);
    unsigned symmetry = stoi(rapidxml::get_attribute(node, "symmetry", "8"));
    unsigned screenshots =
        stoi(rapidxml::get_attribute(node, "screenshots", "2"));
    unsigned width = stoi(rapidxml::get_attribute(node, "width", "48"));
    unsigned height = stoi(rapidxml::get_attribute(node, "height", "48"));

    std::cout << name << " started!" << std::endl;
    // Stop hardcoding samples
    const std::string image_path = "samples/" + name + ".png";
    Array2D<Color> m = read_image(image_path);
    static Array2D<Color> NNN = Array2D<Color>{0, 0};
    if (m == NNN) {
        throw "Error while loading " + image_path;
    }

    OverlappingWFCOptions options = {
        periodic_input, periodic_output, height, width, symmetry, ground, N
    };
    for (unsigned screenshot_idx = 0; screenshot_idx < screenshots; screenshot_idx++) {
        for (unsigned test_idx = 0; test_idx < 10; test_idx++) {
            int seed = get_random_seed();
            OverlappingWFC<Color> wfc(m, options, seed);
            Array2D<Color> success = wfc.run();
            static Array2D<Color> NULL_COLOR_ARR = Array2D<Color>{0, 0};

            if (success != NULL_COLOR_ARR) {
                write_image_png("results/" + name + std::to_string(screenshot_idx) + ".png", success);
                std::cout << name << " finished!" << std::endl;
                break;
            } else {
                std::cout << "ERROR: '" << name << "' failed!" << " x" << test_idx << std::endl;
            }
        }
    }
}

/**
 * Transform a symmetry name into its Symmetry enum
 */
Symmetry to_symmetry(const std::string &symmetry_name) {
    if (symmetry_name == "X") {
        return Symmetry::X;
    }
    if (symmetry_name == "T") {
        return Symmetry::T;
    }
    if (symmetry_name == "I") {
        return Symmetry::I;
    }
    if (symmetry_name == "L") {
        return Symmetry::L;
    }
    if (symmetry_name == "\\") {
        return Symmetry::backslash;
    }
    if (symmetry_name == "P") {
        return Symmetry::P;
    }
    throw symmetry_name + "is an invalid Symmetry";
}

/**
 * Read the names of the tiles in the subset in a tiling WFC problem
 */
std::unordered_set<std::string> read_subset_names(
    xml_node<> *root_node, const std::string &subset )
{
    std::unordered_set<std::string> subset_names;
    xml_node<> *subsets_node = root_node->first_node("subsets");
    if (!subsets_node) {
        return{};
    }
    xml_node<> *subset_node = subsets_node->first_node("subset");
    while (subset_node &&
        rapidxml::get_attribute(subset_node, "name") != subset) {
        subset_node = subset_node->next_sibling("subset");
    }
    if (!subset_node) {
        return{};
    }
    for (xml_node<> *node = subset_node->first_node("tile"); node;
        node = node->next_sibling("tile")) {
        subset_names.insert(rapidxml::get_attribute(node, "name"));
    }
    return subset_names;
}

/**
 * Read all tiles for a tiling problem
 */
std::unordered_map<std::string, Tile<Color>> read_tiles(
    xml_node<> *root_node, const std::string &current_dir,
    const std::string &subset, unsigned size )
{
    std::unordered_set<std::string> subset_names = read_subset_names(root_node, subset);
    std::unordered_map<std::string, Tile<Color>> tiles;
    xml_node<> *tiles_node = root_node->first_node("tiles");
    for (xml_node<> *node = tiles_node->first_node("tile");
        node; node = node->next_sibling("tile")
    )
    {
        std::string name = rapidxml::get_attribute(node, "name");
        if (subset_names.empty() &&
            subset_names.find(name) == subset_names.end()) {
            continue;
        }
        Symmetry symmetry = to_symmetry(rapidxml::get_attribute(node, "symmetry", "X"));
        double weight = stod(rapidxml::get_attribute(node, "weight", "1.0"));
        const std::string image_path = current_dir + "/" + name + ".png";
        Array2D<Color> image = read_image(image_path);

        static auto NNN = Array2D<Color>{0, 0};
        if (image == NNN) {
            std::vector<Array2D<Color>> images;
            for (unsigned i = 0; i < nb_of_possible_orientations(symmetry); i++) {
                const std::string image_path = current_dir + "/" + name + " " + std::to_string(i) + ".png";
                Array2D<Color> image = read_image(image_path);
                if (image == NNN) {
                    throw "Error while loading " + image_path;
                }
                if ((image.width != size) || (image.height != size)) {
                    throw "Image " + image_path + " has wrond size";
                }
                images.push_back(image);
            }
            Tile<Color> tile = {images, symmetry, weight};
            tiles.insert({name, tile});
        } else {
            if ((image.width != size) || (image.height != size)) {
                throw "Image " + image_path + " has wrong size";
            }

            Tile<Color> tile(image, symmetry, weight);
            tiles.insert({name, tile});
        }
    }

    return tiles;
}

/**
 * Read the neighbors constraints for a tiling problem.
 * A value {t1,o1,t2,o2} means that the tile t1 with orientation o1 can be
 * placed at the right of the tile t2 with orientation o2.
 */
std::vector<std::tuple<std::string, unsigned, std::string, unsigned>>
read_neighbors(xml_node<> *root_node) {
    std::vector<std::tuple<std::string, unsigned, std::string, unsigned>> neighbors;
    xml_node<> *neighbor_node = root_node->first_node("neighbors");
    for (xml_node<> *node = neighbor_node->first_node("neighbor"); node;
        node = node->next_sibling("neighbor")) {
        std::string left = rapidxml::get_attribute(node, "left");
        std::string::size_type left_delimiter = left.find(" ");
        std::string left_tile = left.substr(0, left_delimiter);
        unsigned left_orientation = 0;
        if (left_delimiter != std::string::npos) {
            left_orientation = stoi(left.substr(left_delimiter, std::string::npos));
        }

        std::string right = rapidxml::get_attribute(node, "right");
        std::string::size_type right_delimiter = right.find(" ");
        std::string right_tile = right.substr(0, right_delimiter);
        unsigned right_orientation = 0;
        if (right_delimiter != std::string::npos) {
            right_orientation = stoi(right.substr(right_delimiter, std::string::npos));
        }
        neighbors.push_back(
            std::make_tuple(left_tile, left_orientation, right_tile, right_orientation));
    }
    return neighbors;
}

/**
 * Read an instance of a tiling WFC problem.
 */
void read_simpletiled_instance(xml_node<> *node, const std::string &current_dir)
{
    std::string name = rapidxml::get_attribute(node, "name");
    std::string subset = rapidxml::get_attribute(node, "subset", "tiles");
    bool periodic_output =
        (rapidxml::get_attribute(node, "periodic", "False") == "True");
    unsigned width = stoi(rapidxml::get_attribute(node, "width", "48"));
    unsigned height = stoi(rapidxml::get_attribute(node, "height", "48"));

    std::cout << name << " " << subset << " started!" << std::endl;

    std::ifstream config_file("samples/" + name + "/data.xml");
    std::vector<char> buffer((std::istreambuf_iterator<char>(config_file)),
                             std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    xml_document<> data_document;
    data_document.parse<0>(&buffer[0]);
    xml_node<> *data_root_node = data_document.first_node("set");
    unsigned size = stoi(rapidxml::get_attribute(data_root_node, "size"));

    std::unordered_map<std::string, Tile<Color>> tiles_map = read_tiles(data_root_node, current_dir + "/" + name, subset, size);
    std::unordered_map<std::string, unsigned> tiles_id;
    std::vector<Tile<Color>> tiles;
    unsigned id = 0;
    for (std::pair<std::string, Tile<Color>> tile : tiles_map) {
        tiles_id.insert({tile.first, id});
        tiles.push_back(tile.second);
        id++;
    }

    std::vector<std::tuple<std::string, unsigned, std::string, unsigned>> neighbors =
        read_neighbors(data_root_node);
    std::vector<std::tuple<unsigned, unsigned, unsigned, unsigned>> neighbors_ids;
    for (auto neighbor : neighbors) {
        const std::string &neighbor1 = std::get<0>(neighbor);
        const int &orientation1 = std::get<1>(neighbor);
        const std::string &neighbor2 = std::get<2>(neighbor);
        const int &orientation2 = std::get<3>(neighbor);
        if (tiles_id.find(neighbor1) == tiles_id.end()) {
            continue;
        }
        if (tiles_id.find(neighbor2) == tiles_id.end()) {
            continue;
        }
        neighbors_ids.push_back(std::make_tuple(tiles_id[neighbor1], orientation1,
                                                tiles_id[neighbor2], orientation2));
    }

    assert(tiles.size() > 0);
    for (unsigned test = 0; test < 10; test++) {
        int seed = get_random_seed();
        TilingWFC<Color> wfc(
            tiles, neighbors_ids, height, width, {periodic_output}, seed
        );
        Array2D<Color> success = wfc.run();

        static auto NNN = Array2D<Color>{0, 0};
        if (success != NNN) {
            write_image_png("results/" + name + "_" + subset + ".png", success);
            std::cout << name << " finished!" << std::endl;
            break;
        }
        else {
            std::cout << "failed!" << std::endl;
        }
    }
}

/**
 * Read a configuration file containing multiple wfc problems
 */
void read_config_file(const std::string &config_path)  {
    std::ifstream config_file(config_path);
    std::vector<char> buffer((std::istreambuf_iterator<char>(config_file)),
                             std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    xml_document<> document;
    document.parse<0>(&buffer[0]);

    xml_node<> *root_node = document.first_node("samples");
    std::string dir_path = get_dir(config_path) + "/" + "samples";
    for (xml_node<> *node = root_node->first_node("overlapping"); node;
        node = node->next_sibling("overlapping")) {
        read_overlapping_instance(node);
    }
    for (xml_node<> *node = root_node->first_node("simpletiled");
        node;
        node = node->next_sibling("simpletiled")
        ) {
        read_simpletiled_instance(node, dir_path);
    }
}

int main() {

    // Initialize rand for non-linux targets
#ifndef __linux__
    srand((int)time(nullptr));
#endif

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    read_config_file("samples.xml");

    end = std::chrono::system_clock::now();
    int elapsed_s =
        (int)std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    int elapsed_ms =
        (int)std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count();
    std::cout << "All samples done in " << elapsed_s << "s, " << elapsed_ms % 1000
        << "ms.\n";

    return 0;
}
