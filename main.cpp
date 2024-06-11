#include <fstream>
#include <iostream>
#include <limits>
#include <png.h>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

class Image {
private:
    string inputFileName;
    string outputFileName;
    vector<unsigned char> pixels;  // Use vector instead of raw pointers
    vector<char> asciiImage;  // Use vector instead of raw pointers
    int width;
    int height;

    // ASCII density characters
    static constexpr char densityChars[] = ".`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    static const int densitySize = sizeof(densityChars) - 1;

    bool readFooFile() {
        ifstream inputFile(inputFileName, ios::binary);
        if (!inputFile.is_open()) {
            throw runtime_error("Failed to open input file: " + inputFileName);
        }

        inputFile >> width >> height;
        inputFile.ignore(numeric_limits<streamsize>::max(), '\n');

        pixels.resize(width * height);
        inputFile.read(reinterpret_cast<char *>(pixels.data()), width * height);

        if (inputFile.fail()) {
            throw runtime_error("Failed to read pixels from file: " + inputFileName);
        }

        inputFile.close();
        return true;
    }

    bool readPngFile() {
        FILE *fp = fopen(inputFileName.c_str(), "rb");
        if (!fp) {
            throw runtime_error("Failed to open input file: " + inputFileName);
        }

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            fclose(fp);
            throw runtime_error("Failed to create PNG read struct");
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            fclose(fp);
            throw runtime_error("Failed to create PNG info struct");
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            throw runtime_error("PNG read error");
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        png_byte color_type = png_get_color_type(png, info);
        png_byte bit_depth = png_get_bit_depth(png, info);

        if (bit_depth == 16) {
            png_set_strip_16(png);
        }

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png);
        }

        if (png_get_valid(png, info, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png);
        }

        if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png);
        }

        png_read_update_info(png, info);

        vector<png_bytep> row_pointers(height);
        for (int y = 0; y < height; y++) {
            row_pointers[y] = new png_byte[png_get_rowbytes(png, info)];
        }

        png_read_image(png, row_pointers.data());

        fclose(fp);
        png_destroy_read_struct(&png, &info, nullptr);

        pixels.resize(width * height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                png_bytep px = &(row_pointers[y][x * 4]);
                pixels[y * width + x] = 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2];
            }
            delete[] row_pointers[y];
        }

        return true;
    }

    unsigned char bilinearInterpolation(float x, float y) {
        int x1 = floor(x);
        int y1 = floor(y);
        int x2 = ceil(x);
        int y2 = ceil(y);

        float xWeight = x - x1;
        float yWeight = y - y1;

        unsigned char top = (1 - xWeight) * pixels[y1 * width + x1] + xWeight * pixels[y1 * width + x2];
        unsigned char bottom = (1 - xWeight) * pixels[y2 * width + x1] + xWeight * pixels[y2 * width + x2];
        return static_cast<unsigned char>((1 - yWeight) * top + yWeight * bottom);
    }

    void convertPixelsToAscii() {
        float aspectRatio = static_cast<float>(width) / height;
        int targetWidth = static_cast<int>(width / 2);
        int targetHeight = static_cast<int>(targetWidth / aspectRatio);

        asciiImage.resize(targetWidth * targetHeight + targetHeight);

        for (int row = 0; row < targetHeight; ++row) {
            for (int col = 0; col < targetWidth; ++col) {
                float pixelX = col * width / static_cast<float>(targetWidth);
                float pixelY = row * height / static_cast<float>(targetHeight);
                int pixelValue = bilinearInterpolation(pixelX, pixelY);
                int asciiIndex = pixelValue * densitySize / 256;
                asciiImage[row * (targetWidth + 1) + col] = densityChars[asciiIndex];
            }
            asciiImage[row * (targetWidth + 1) + targetWidth] = '\n';
        }
    }

    bool writeFoo2File() {
        ofstream outputFile(outputFileName);
        if (!outputFile.is_open()) {
            throw runtime_error("Failed to open output file: " + outputFileName);
        }

        int targetWidth = width / 2; // Correspondente à largura da arte ASCII gerada
        int targetHeight = static_cast<int>(targetWidth / (static_cast<float>(width) / height)); // Correspondente à altura da arte ASCII gerada
        outputFile << targetWidth << " " << targetHeight << "\n";
        outputFile.write(asciiImage.data(), asciiImage.size());

        outputFile.close();
        return true;
    }

    bool writeFooFile() {
        ofstream outputFile(outputFileName, ios::binary);
        if (!outputFile.is_open()) {
            throw runtime_error("Failed to open output file: " + outputFileName);
        }

        outputFile << width << " " << height << "\n";
        outputFile.write(reinterpret_cast<char *>(pixels.data()), width * height);

        outputFile.close();
        return true;
    }

public:
    Image(const string &inputFileName, const string &outputFileName)
        : inputFileName(inputFileName), outputFileName(outputFileName), width(0), height(0) {}

    bool convertToFoo2() {
        if (!readFooFile()) {
            cerr << "Failed to read input file: " << inputFileName << endl;
            return false;
        }

        convertPixelsToAscii();

        if (!writeFoo2File()) {
            cerr << "Failed to write output file: " + outputFileName << endl;
            return false;
        }

        return true;
    }

    bool convertPngToFoo() {
        if (!readPngFile()) {
            cerr << "Failed to read input file: " << inputFileName << endl;
            return false;
        }

        if (!writeFooFile()) {
            cerr << "Failed to write output file: " << outputFileName << endl;
            return false;
        }

        return true;
    }

    int getWidth() { return width; }

    int getHeight() { return height; }
};

// Definição dos caracteres de densidade
constexpr char Image::densityChars[];

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <mode> <input_file> <output_file>" << endl;
        cerr << "Modes: foo2foo2, png2foo" << endl;
        return -1;
    }

    string mode = argv[1];
    string inputFileName = argv[2];
    string outputFileName = argv[3];

    Image conversor(inputFileName, outputFileName);

    if (mode == "foo2foo2") {
        if (!conversor.convertToFoo2()) {
            return -1;
        }
    } else if (mode == "png2foo") {
        if (!conversor.convertPngToFoo()) {
            return -1;
        }
    } else {
        cerr << "Invalid mode: " << mode << endl;
        return -1;
    }

    return 0;
}
