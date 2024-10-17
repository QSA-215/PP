#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <cmath>
#include <algorithm>
#include <string>

#include <cstring>
#include <cassert>
#include <stdexcept>

#pragma pack(push, 1) // отключение выравнивания структуры
struct BMPHeader
{
    uint16_t file_type{ 0x4D42 };
    uint32_t file_size{ 0 };
    uint16_t reserved1{ 0 };
    uint16_t reserved2{ 0 };
    uint32_t offset_data{ 0 };
};

struct BMPInfoHeader
{
    uint32_t size{ 0 };
    int32_t width{ 0 };
    int32_t height{ 0 };
    uint16_t planes{ 1 };
    uint16_t bit_count{ 0 };
    uint32_t compression{ 0 };
    uint32_t size_image{ 0 };
    int32_t x_pixels_per_meter{ 0 };
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 };
    uint32_t colors_important{ 0 };
};

struct BMPColorHeader
{
    uint32_t red_mask{ 0x00ff0000 };
    uint32_t green_mask{ 0x0000ff00 };
    uint32_t blue_mask{ 0x000000ff };
    uint32_t alpha_mask{ 0xff000000 };
    uint32_t color_space_type{ 0x73524742 };
    uint32_t unused[16]{ 0 };
};
#pragma pack(pop)

struct BMPImage
{
    BMPHeader file_header;
    BMPInfoHeader bmp_info_header;
    BMPColorHeader bmp_color_header;
    std::vector<uint8_t> data;

    BMPImage() = default;

    BMPImage(const std::string& file_name)
    {
        read(file_name);
    }

    void read(const std::string& file_name)
    {
        std::ifstream inp(file_name, std::ios::binary);
        if (!inp)
        {
            throw std::runtime_error("Error opening the file " + file_name);
        }

        inp.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
        if (file_header.file_type != 0x4D42)
        {
            throw std::runtime_error("Wrong file format. Expected BMP.");
        }

        inp.read(reinterpret_cast<char*>(&bmp_info_header), sizeof(bmp_info_header));

        if (bmp_info_header.bit_count == 32)
        {
            inp.read(reinterpret_cast<char*>(&bmp_color_header), sizeof(bmp_color_header));
        }

        inp.seekg(file_header.offset_data, inp.beg);

        size_t row_padded = (bmp_info_header.width * bmp_info_header.bit_count / 8 + 3) & (~3);
        data.resize(row_padded * bmp_info_header.height);
        inp.read(reinterpret_cast<char*>(data.data()), data.size());
    }

    void write(const std::string& file_name) const
    {
        std::ofstream of(file_name, std::ios::binary);
        if (!of)
        {
            throw std::runtime_error("Writing error " + file_name);
        }

        of.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
        of.write(reinterpret_cast<const char*>(&bmp_info_header), sizeof(bmp_info_header));
        if (bmp_info_header.bit_count == 32)
        {
            of.write(reinterpret_cast<const char*>(&bmp_color_header), sizeof(bmp_color_header));
        }

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
};

struct ThreadParams
{
    BMPImage input;
    BMPImage& output;
    int start;
    int end;
    int power;
};

DWORD WINAPI ThreadProc(LPVOID params)
{
    ThreadParams threadParams = *(ThreadParams*)params;

    int width = threadParams.input.bmp_info_header.width;
    int height = threadParams.input.bmp_info_header.height;
    int bytesPerPixel = threadParams.input.bmp_info_header.bit_count / 8;

    size_t rowPadded = (width * bytesPerPixel + 3) & (~3);

    for (int y = threadParams.start; y < threadParams.end; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;

            // Блюр размера (1+radius)x(1+radius)
            for (int ky = -threadParams.power; ky <= threadParams.power; ++ky)
            {
                for (int kx = -threadParams.power; kx <= threadParams.power; ++kx)
                {
                    int nx = std::clamp(x + kx, 0, width - 1);
                    int ny = std::clamp(y + ky, 0, height - 1);

                    size_t index = ny * rowPadded + nx * bytesPerPixel;
                    uint8_t b = threadParams.input.data[index];
                    uint8_t g = threadParams.input.data[index + 1];
                    uint8_t r = threadParams.input.data[index + 2];

                    sum_r += r;
                    sum_g += g;
                    sum_b += b;
                    count++;
                }
            }

            size_t out_index = y * rowPadded + x * bytesPerPixel;
            threadParams.output.data[out_index] = static_cast<uint8_t>(sum_b / count);
            threadParams.output.data[out_index + 1] = static_cast<uint8_t>(sum_g / count);
            threadParams.output.data[out_index + 2] = static_cast<uint8_t>(sum_r / count);
            if (bytesPerPixel == 4)
            {
                threadParams.output.data[out_index + 3] = threadParams.input.data[out_index + 3];
            }
        }
    }
    ExitThread(0);
}

void MultithreadBlur(const BMPImage& input, BMPImage& output, int threadsAmount, int coresAmount, int power)
{
    HANDLE* threads = new HANDLE[threadsAmount];
    int rowsPerThread = input.bmp_info_header.height / threadsAmount;
    int remainingRows = input.bmp_info_header.height % threadsAmount;

    int start = 0;

    for (int i = 0; i < threadsAmount; i++)
    {
        int end = start + rowsPerThread;
        if (i == threadsAmount - 1)
            end += remainingRows;

        ThreadParams* params = new ThreadParams { input, output, start, end, power };
        LPVOID lpData;
        lpData = (LPVOID)params;

        threads[i] = CreateThread(NULL, 0, &ThreadProc, lpData, CREATE_SUSPENDED, NULL);
        SetThreadAffinityMask(GetCurrentThread(), (1 << coresAmount) - 1);

        start = end;
    }

    for (int i = 0; i < threadsAmount; i++)
        ResumeThread(threads[i]);

    WaitForMultipleObjects(threadsAmount, threads, true, INFINITE);
}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        std::cout << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <number of cores> <number of threads> <blur power>" << std::endl;
        return 1;
    }

    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];
    int coresAmount = std::stoi(argv[3]);
    int threadsAmount = std::stoi(argv[4]);
    int power = std::stoi(argv[5]);

    clock_t timeStart = clock();
    try
    {
        BMPImage input(inputFileName);
        BMPImage output = input;

        MultithreadBlur(input, output, threadsAmount, coresAmount, power);

        output.write(outputFileName);
    }
    catch (const std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    clock_t timeEnd = clock();

    //std::cout << (timeEnd - timeStart) * 1000.0 / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << (timeEnd - timeStart) * 1000.0 / CLOCKS_PER_SEC;
    return 0;
}