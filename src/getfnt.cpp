#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static bool write_fnts_to_disk(std::ifstream &file,
                               const std::uint32_t rt_address,
                               const std::string &out_name) {
  file.seekg(rt_address);

  std::uint16_t shift{0x0};
  file.read(reinterpret_cast<char *>(&shift), sizeof(shift));

  /// Iterate over resource table
  while (true) {
    std::uint16_t type_id{0x0};
    file.read(reinterpret_cast<char *>(&type_id), sizeof(type_id));

    if (type_id == 0) {
      break; /// End of resource table
    }

    std::uint16_t res_count{0};
    file.read(reinterpret_cast<char *>(&res_count), sizeof(res_count));

    /// Skip reserved dword
    file.seekg(4, std::ios::cur);

    for (auto i = 0u; i < res_count; ++i) {
      std::uint16_t start{0x0};
      file.read(reinterpret_cast<char *>(&start), sizeof(start));
      start = start << shift;

      std::uint16_t size{0};
      file.read(reinterpret_cast<char *>(&size), sizeof(size));
      size = size << shift;

      if (start < 0 || size < 0) { /// Should also probably check start + size <
                                   /// file_size as well
        std::cerr << "Invalid .fnt start/size\n";
        return false;
      }

      if (type_id == 0x8008) { /// RT_FONT type
        std::uint32_t pos = file.tellg();
        file.seekg(start);
        std::vector<char> data(size);
        file.read(data.data(), size);

        std::string file_name = out_name + "_" + std::to_string(i) + ".fnt";
        std::ofstream out_file(file_name, std::ios::binary);
        if (out_file) {
          out_file.write(data.data(), size);
          std::cout << "Saved " << file_name << " (" << size << "bytes)\n";
        } else {
          std::cerr << "Error saving " << file_name << std::endl;
        }

        file.seekg(pos + 8);
      } else {
        file.seekg(res_count * 8, std::ios::cur);
      }
    }
  }

  return true;
}

std::uint32_t get_ne_offset(std::ifstream &file) {
  std::uint32_t ne_offset{0x0};

  /// NE header offset is at MZ (0x0) + 0x3C
  file.seekg(0x3C);
  file.read(reinterpret_cast<char *>(&ne_offset), sizeof(ne_offset));
  return ne_offset;
}

std::uint16_t get_rt_offset(std::ifstream &file,
                            const std::uint32_t ne_offset) {
  std::uint16_t rt_offset{0x0};

  /// Resource table is at NE header + 0x24
  file.seekg(ne_offset + 0x24);
  file.read(reinterpret_cast<char *>(&rt_offset), sizeof(rt_offset));
  return rt_offset;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: getfnt <FON_PATH> <OUTPUT_NAME>\n";
    return -1;
  }

  const std::string fon_name = argv[1];
  const std::string out_name = argv[2];

  std::ifstream file(fon_name, std::ios::binary);
  if (!file || !file.good()) {
    std::cerr << "Failed to open .fon file\n";
    return -1;
  }

  std::uint32_t ne_offset{get_ne_offset(file)};
  std::uint16_t rt_offset{get_rt_offset(file, ne_offset)};

  if (!write_fnts_to_disk(file, ne_offset + rt_offset, out_name)) {
    std::cerr << "Failed\n";
    return -2;
  }

  return 0;
}
