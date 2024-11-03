#include "xx_auto.hpp"
#undef min
#undef max

#include <fstream>
#include <onnxruntime_cxx_api.h>

namespace Utils {
    using namespace xx_auto;

    // anticlockwise
    void minAreaRect(const std::vector<point> &convex_hull, pointf rect[4]) {
        rect[0].x = rect[3].x = convex_hull[0].x;
        rect[0].y = rect[3].y = convex_hull[0].y;
        rect[1].x = rect[2].x = convex_hull[1].x;
        rect[1].y = rect[2].y = convex_hull[1].y;
        float min_area = -1;
        float min_wlx = rect[0].x, min_wly = rect[0].y, min_wrx = rect[1].x, min_wry = rect[1].y, min_hx = 0, min_hy = 0;
        for (std::vector<point>::size_type size = convex_hull.size(), index = 1; index < size; ++index) {
            /*
                ab ap aq
                ab * ap = |ab| * |ap| * cos(alpha)
                |aq| = |ap| * cos(alpha)
                |aq| / |ab| = |ap| * cos(alpha) / |ab| = ab * ap / (|ab| * |ab|)
                S = |ab| * |qp|
             */
            const auto &a = convex_hull[index - 1], &b = convex_hull[index];
            float ab2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
            float abx = b.x - a.x, aby = b.y - a.y;
            float wlx = a.x, wly = a.y, wrx = b.x, wry = b.y, hx = 0, hy = 0;
            float dl = 0, dr = 1;
            for (auto offset = 1; offset < size - 1; ++offset) {
                const auto &p = convex_hull[(index + offset) % size];
                auto d = (abx * (p.x - a.x) + aby * (p.y - a.y)) / ab2;
                auto qx = a.x + d * abx, qy = a.y + d * aby;
                auto qpx = p.x - qx, qpy = p.y - qy;

                if (qpx * qpx + qpy * qpy > hx * hx + hy * hy)
                    hx = qpx, hy = qpy;

                if (d < dl)
                    dl = d, wlx = qx, wly = qy;
                else if (d > dr)
                    dr = d, wrx = qx, wry = qy;
            }
            auto area = std::sqrt((wrx - wlx) * (wrx - wlx) + (wry - wly) * (wry - wly)) * std::sqrt(hx * hx + hy * hy);
            if (min_area < 0 || area < min_area) {
                min_area = area;
                min_wlx = wlx, min_wly = wly, min_wrx = wrx, min_wry = wry, min_hx = hx, min_hy = hy;
            }
        }
        rect[0].x = min_wlx, rect[0].y = min_wly;
        rect[1].x = min_wrx, rect[1].y = min_wry;
        rect[2].x = min_wrx + min_hx, rect[2].y = min_wry + min_hy;
        rect[3].x = min_wlx + min_hx, rect[3].y = min_wly + min_hy;
    }
    enum class ContourType {
        Hull,
        Convex,
        Rect,
    };
    // anticlockwise
    std::vector<std::vector<point>> contours(image &binary_image, ContourType contour_type = ContourType::Hull) {
        std::vector<std::vector<point>> contours;

        auto width = binary_image.width, height = binary_image.height;
        auto &data = binary_image.data;
        /*
            0 7 6
            1 x 5
            2 3 4
        */
        int direction[] = {-1, -1, -1, 0, -1, 1, 0, 1, 1, 1, 1, 0, 1, -1, 0, -1};
        for (auto y = 0; y < height; ++y) {
            for (auto x = 0; x < width; ++x) {
                auto index = static_cast<std::size_t>(y) * width + x;
                if (data[index] == 1 && (x == 0 || data[index - 1] == 0)) {
                    std::vector<point> contour;
                    for (auto cur_x = x - 1, cur_y = y - 1, cur_dir = 4, step = 0; step < 8; ++step) {
                        auto dir = direction + (cur_dir + step) * 2 % 16;
                        auto next_x = cur_x + *dir, next_y = cur_y + *(dir + 1);
                        if ((next_x >= 0 && next_x < width && next_y >= 0 && next_y < height)) {
                            auto &value = data[next_y * width + next_x];
                            if (value != 0) {
                                auto contour_size = contour.size();
                                if (next_x == x && next_y == y && contour_size > 0)
                                    break;
                                if (value == 1) {
                                    value = 2;
                                    point pt{next_x, next_y};
                                    switch (contour_type) {
                                    case ContourType::Rect:
                                        if (contour_size == 0) {
                                            contour.push_back(pt);
                                            contour.push_back(pt);
                                            contour.push_back(pt);
                                            contour.push_back(pt);
                                        } else {
                                            if (pt.x > contour[2].x)
                                                contour[2].x = contour[3].x = pt.x;
                                            else if (pt.x < contour[0].x)
                                                contour[0].x = contour[1].x = pt.x;
                                            if (pt.y > contour[2].y)
                                                contour[1].y = contour[2].y = pt.y;
                                            else if (pt.y < contour[0].y)
                                                contour[0].y = contour[3].y = pt.y;
                                        }
                                        break;
                                    case ContourType::Convex:
                                        for (decltype(contour_size) index = 0; index < contour_size; ++index) {
                                            auto peek = index + 1;
                                            while (peek < contour_size) {
                                                const auto &l = contour[index], &m = contour[peek], &r = pt;
                                                if ((l.x - m.x) * (r.y - m.y) - (l.y - m.y) * (r.x - m.x) > 0)
                                                    break;
                                                ++peek;
                                            }
                                            if (peek == contour_size) {
                                                contour.resize(index + 1);
                                                break;
                                            }
                                        }
                                    default:
                                        contour.push_back(pt);
                                        break;
                                    }
                                }
                                cur_x = next_x, cur_y = next_y, cur_dir = (cur_dir + step + 5) % 8, step = 0;
                            }
                        }
                    }
                    contours.push_back(std::move(contour));
                }
            }
        }
        return contours;
    }
    void rotate180(image &input) {
        for (auto width = input.width, height = input.height, channel = input.channel, end_y = height / 2 + height % 2, y = 0; y < end_y; ++y) {
            for (auto end_x = y < height / 2 ? width : width / 2, x = 0; x < end_x; ++x) {
                for (auto c = 0; c < channel; ++c)
                    std::swap(input.data[(y * width + x) * channel + c], input.data[((height - 1 - y) * width + (width - 1 - x)) * channel + c]);
            }
        }
    }

    void input_data(const image &rgb_input, int input_width, int input_height, const float mean[3], const float norm[3], float *data) {
        auto input_size = static_cast<std::size_t>(input_width) * input_height * 3;
        for (auto width = rgb_input.width, height = rgb_input.height, channel = rgb_input.channel, y = 0; y < height; ++y) {
            for (auto x = 0; x < width; ++x) {
                for (int ch = 0; ch < 3; ++ch) {
                    auto index = input_width * input_height * ch + y * input_width + x;
                    if (index < input_size) {
                        auto value = (rgb_input.data[(y * width + x) * channel + ch] / 255.0 - mean[ch]) / norm[ch];
                        data[index] = static_cast<float>(value);
                    }
                }
            }
        }
    }
    void split_with_rotate(const image &input, image &binary, int side_limit, float up_threshold, float box_threshold, float unclip_ratio, float width_ratio, float height_ratio, std::vector<point> &boxes, std::vector<image> &images) {
        pointf rect[4];
        float offset, width, height, origin_x, origin_y;
        pointf horizontal, vertical;
        for (const auto &contour : Utils::contours(binary, Utils::ContourType::Convex)) {
            if (contour.size() < 3)
                continue;
            Utils::minAreaRect(contour, rect); // anticlockwise

            auto ul_index = 0;
            for (auto index = 0; index < 4; ++index) {
                if (rect[index].y < rect[ul_index].y - up_threshold || (rect[index].y < rect[ul_index].y + up_threshold && rect[index].x < rect[ul_index].x))
                    ul_index = index;
            }
            horizontal.x = rect[(ul_index + 3) % 4].x - rect[ul_index].x, horizontal.y = rect[(ul_index + 3) % 4].y - rect[ul_index].y;
            vertical.x = rect[(ul_index + 1) % 4].x - rect[ul_index].x, vertical.y = rect[(ul_index + 1) % 4].y - rect[ul_index].y;
            width = std::sqrt(horizontal.x * horizontal.x + horizontal.y * horizontal.y), height = std::sqrt(vertical.x * vertical.x + vertical.y * vertical.y);
            if (width < side_limit || height < side_limit)
                continue;

            horizontal.x /= width, horizontal.y /= width;
            vertical.x /= height, vertical.y /= height;
            if (width * box_threshold < height) {
                ul_index = (ul_index + 1) % 4;
                std::swap(width, height);
                std::swap(horizontal.x, vertical.x);
                std::swap(horizontal.y, vertical.y);
                horizontal.x *= -1, horizontal.y *= -1;
            }
            offset = width * height / (width + height) * unclip_ratio;
            origin_x = (rect[ul_index].x - (horizontal.x + vertical.x) * offset / 2) * width_ratio, origin_y = (rect[ul_index].y - (horizontal.y + vertical.y) * offset / 2) * height_ratio;
            width = (width + offset) * width_ratio, height = (height + offset) * height_ratio;

            // clockwise
            boxes.push_back({static_cast<int>(origin_x), static_cast<int>(origin_y)});
            boxes.push_back({static_cast<int>(std::ceil(horizontal.x * width)), static_cast<int>(std::ceil(horizontal.y * width))});
            boxes.push_back({static_cast<int>(std::ceil(vertical.x * height)), static_cast<int>(std::ceil(vertical.y * height))});
            images.push_back(input.scoop(static_cast<int>(origin_x), static_cast<int>(origin_y), static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)), horizontal, vertical));
        }
    }
    void split(const image &input, image &binary, int side_limit, float up_threshold, float box_threshold, float unclip_ratio, float width_ratio, float height_ratio, std::vector<point> &boxes, std::vector<image> &images) {
        float offset, width, height, origin_x, origin_y;
        for (const auto &rect : Utils::contours(binary, Utils::ContourType::Rect)) {
            int width = rect[2].x - rect[0].x, height = rect[2].y - rect[0].y;
            if (width < side_limit || height < side_limit)
                continue;

            offset = width * height / (width + height) * unclip_ratio;
            origin_x = (rect[0].x - offset / 2) * width_ratio, origin_y = (rect[0].y - offset / 2) * height_ratio;
            width = (width + offset) * width_ratio, height = (height + offset) * height_ratio;

            // clockwise
            boxes.push_back({static_cast<int>(origin_x), static_cast<int>(origin_y)});
            boxes.push_back({static_cast<int>(std::ceil(width)), 0});
            boxes.push_back({0, static_cast<int>(std::ceil(height))});
            images.push_back(input.scoop(static_cast<int>(origin_x), static_cast<int>(origin_y), static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height))));
        }
    }
} // namespace Utils
namespace xx_auto {
    class OcrxOnnx final {
        class Segment {
        protected:
            Ort::Session session;
            std::vector<Ort::AllocatedStringPtr> input_names_ptr;
            std::vector<Ort::AllocatedStringPtr> output_names_ptr;
            std::vector<const char *> input_names;
            std::vector<const char *> output_names;

        public:
            Segment(const Ort::Env &env, const Ort::SessionOptions &options, const ORTCHAR_T *path) : session(env, path, options) {
                Ort::AllocatorWithDefaultOptions allocator;
                for (std::size_t count = session.GetInputCount(), index = 0; index < count; ++index) {
                    input_names_ptr.push_back(std::move(session.GetInputNameAllocated(index, allocator)));
                    input_names.push_back(input_names_ptr[index].get());
                }
                for (std::size_t count = session.GetOutputCount(), index = 0; index < count; index++) {
                    output_names_ptr.push_back(std::move(session.GetOutputNameAllocated(index, allocator)));
                    output_names.push_back(output_names_ptr[index].get());
                }
            }
        };
        class Detector : public Segment {
        private:
            const float mean[3]{0.485f, 0.456f, 0.406f}, norm[3]{0.229f, 0.224f, 0.225f};

        public:
            const int input_side_limit = 960, box_side_limit = 3;
            float binary_threshold = 0.3f, up_threshold = 5.0f, box_threshold = 1.5f, unclip_ratio = 2.0f;

            using Segment::Segment;
            void process(const image &rgb_input, std::vector<point> &boxes, std::vector<image> &images, bool rotated = false) {
                const auto input_image = rgb_input.limit(input_side_limit, input_side_limit, true, true);
                const auto input_width = std::max(static_cast<int>(std::round(input_image.width / 32.0) * 32), 32);
                const auto input_height = std::max(static_cast<int>(std::round(input_image.height / 32.0) * 32), 32);
                auto input_data = std::vector<float>(static_cast<std::vector<float>::size_type>(input_width) * input_height * 3, 0);
                Utils::input_data(input_image, input_width, input_height, mean, norm, input_data.data());

                int64_t input_shape[4] = {1, 3, input_height, input_width};
                auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
                auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), input_shape, sizeof(input_shape) / sizeof(input_shape[0]));
                auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, input_names.size(), output_names.data(), output_names.size());
                auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
                const auto output_data = output_tensors[0].GetTensorMutableData<float>();

                const auto width_ratio = static_cast<float>(rgb_input.width) / input_image.width;
                const auto height_ratio = static_cast<float>(rgb_input.height) / input_image.height;
                image output_image(output_shape[3], output_shape[2], 1);
                for (std::vector<unsigned char>::size_type size = output_image.data.size(), index = 0; index < size; ++index) {
                    unsigned char data = output_data[index] > binary_threshold ? 1 : 0;
                    output_image.data[index] = data;
                }
                auto split = rotated ? Utils::split_with_rotate : Utils::split;
                split(rgb_input, output_image, box_side_limit, up_threshold, box_threshold, unclip_ratio, width_ratio, height_ratio, boxes, images);
            }
        };
        class Classifier : public Segment {
        private:
            const float mean[3]{0.5f, 0.5f, 0.5f}, norm[3]{0.5f, 0.5f, 0.5f};
            const int batch = 1, input_width = 192, input_height = 48;

        public:
            float threshold = 0.8f;

            using Segment::Segment;
            void process(std::vector<image> &rgb_inputs) {
                auto input_data = std::vector<float>(input_width * input_height * 3 * rgb_inputs.size(), 0);
                for (std::vector<image>::size_type size = rgb_inputs.size(), index = 0; index < size; index += batch) {
                    auto pages = std::min<decltype(size)>(batch, size - index);
                    auto input_data_ptr = input_data.data() + input_width * input_height * 3 * index;
                    for (auto page = 0; page < pages; ++page) {
                        const auto input_image = rgb_inputs[index + page].limit(input_width, input_height, true);
                        Utils::input_data(input_image, input_width, input_height, mean, norm, input_data_ptr + input_width * input_height * 3 * page);
                    }

                    int64_t input_shape[4]{static_cast<int64_t>(pages), 3, input_height, input_width};
                    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
                    auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data_ptr, input_width * input_height * 3 * pages, input_shape, sizeof(input_shape) / sizeof(input_shape[0]));
                    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, input_names.size(), output_names.data(), output_names.size());
                    auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
                    auto output_data = output_tensors[0].GetTensorMutableData<float>();

                    for (decltype(size) pages = output_shape[0], length = output_shape[1], page = 0; page < pages; ++page) {
                        int label = 0;
                        float score = 0.0f;
                        for (decltype(size) index = 0; index < length; ++index) {
                            auto value = output_data[length * page + index];
                            if (value > score) {
                                score = value;
                                label = index;
                            }
                        }
                        if (label == 1 && score > threshold)
                            Utils::rotate180(rgb_inputs[index + page]);
                    }
                }
            }
        };
        class Recognizer : public Segment {
        private:
            const float mean[3]{0.5f, 0.5f, 0.5f}, norm[3]{0.5f, 0.5f, 0.5f};
            const int batch = 1, input_width = 460, input_height = 48;
            std::vector<std::string> characters;

            void output_characters(const float *input_data, int64_t map, int64_t length, float &score, std::string &text) const {
                auto count = 0;
                for (decltype(length) last_label = 0, len = 0; len < length; ++len) {
                    decltype(map) label = 0;
                    auto value = -1.f;
                    auto map_data = input_data + map * len;
                    for (decltype(map) index = 0; index < map; ++index) {
                        auto data = map_data[index];
                        if (data > value) {
                            value = data;
                            label = index;
                        }
                    }
                    if (label > 0 && label != last_label) {
                        text += characters[label - 1];
                        score += value;
                        count += 1;
                    }
                    last_label = label;
                }
                score = count > 0 ? score / count : -1;
            }

        public:
            Recognizer(const Ort::Env &env, const Ort::SessionOptions &options, const ORTCHAR_T *path, const ORTCHAR_T *characters_path) : Segment(env, options, path) {
                std::ifstream file(characters_path);
                std::string line;
                while (std::getline(file, line))
                    characters.push_back(line);
                characters.push_back(" ");
            }
            void process(const image &rgb_input, float &score, std::string &text) {
                const auto input_image = rgb_input.limit(input_width, input_height, true);
                auto input_data = std::vector<float>(static_cast<std::vector<float>::size_type>(input_width) * input_height * 3, 0);
                Utils::input_data(input_image, input_width, input_height, mean, norm, input_data.data());

                int64_t input_shape[4]{1, 3, input_height, input_width};
                auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
                auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), input_shape, sizeof(input_shape) / sizeof(input_shape[0]));
                auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, input_names.size(), output_names.data(), output_names.size());
                auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
                auto output_data = output_tensors[0].GetTensorMutableData<float>();

                output_characters(output_data, output_shape[2], output_shape[1], score, text);
            }
            void process(const std::vector<image> &rgb_inputs, std::vector<float> &scores, std::vector<std::string> &texts) {
                auto input_data = std::vector<float>(input_width * input_height * 3 * rgb_inputs.size(), 0);
                for (std::vector<image>::size_type size = rgb_inputs.size(), index = 0; index < size; index += batch) {
                    auto pages = std::min<decltype(size)>(batch, size - index);
                    auto input_data_ptr = input_data.data() + input_width * input_height * 3 * index;
                    for (auto page = 0; page < pages; ++page) {
                        const auto input_image = rgb_inputs[index + page].limit(input_width, input_height, true);
                        Utils::input_data(input_image, input_width, input_height, mean, norm, input_data_ptr + input_width * input_height * 3 * page);
                    }

                    int64_t input_shape[4]{static_cast<int64_t>(pages), 3, input_height, input_width};
                    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
                    auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data_ptr, input_width * input_height * 3 * pages, input_shape, sizeof(input_shape) / sizeof(input_shape[0]));
                    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, input_names.size(), output_names.data(), output_names.size());
                    auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
                    auto output_data = output_tensors[0].GetTensorMutableData<float>();

                    for (decltype(size) pages = output_shape[0], length = output_shape[1], map = output_shape[2], page = 0; page < pages; ++page) {
                        auto score = 0.0f;
                        std::string text;
                        output_characters(output_data + map * length * page, map, length, score, text);
                        scores.push_back(score);
                        texts.push_back(std::move(text));
                    }
                }
            }
        };

    private:
        Ort::Env env;
        Ort::SessionOptions options;
        Detector *detector;
        Classifier *classifier;
        Recognizer *recognizer;

    public:
        OcrxOnnx(const ORTCHAR_T *det_path, const ORTCHAR_T *cls_path, const ORTCHAR_T *rec_path, const ORTCHAR_T *characters_path) : env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, "Ocrx") {
            // try
            // {
            //     OrtCUDAProviderOptions cuda_options;
            //     cuda_options.device_id = 0;
            //     cuda_options.arena_extend_strategy = 0;
            //     cuda_options.gpu_mem_limit = SIZE_MAX;
            //     cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearch::OrtCudnnConvAlgoSearchExhaustive;
            //     cuda_options.do_copy_in_default_stream = 1;
            //     options.AppendExecutionProvider_CUDA(cuda_options);
            // }
            // catch (const std::exception &e)
            // {
            //     std::cerr << e.what();
            // }
            options.SetIntraOpNumThreads(4);
            options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            detector = new Detector(env, options, det_path);
            classifier = new Classifier(env, options, cls_path);
            recognizer = new Recognizer(env, options, rec_path, characters_path);
        }
        ~OcrxOnnx() {
            delete detector;
            delete classifier;
            delete recognizer;
        }

        std::vector<ocr_result> recognizes(const image &rgb_image, bool rotated = false) const {
            std::vector<point> boxes;
            std::vector<image> images;
            std::vector<float> scores;
            std::vector<std::string> texts;
            std::vector<ocr_result> results;
            detector->process(rgb_image, boxes, images, rotated);
            if (rotated)
                classifier->process(images);
            recognizer->process(images, scores, texts);
            for (int index = 0; index < images.size(); ++index) {
                if (scores[index] < 0)
                    continue;
                ocr_result result;
                result.box[0] = boxes[index * 3];
                result.box[1] = boxes[index * 3 + 1];
                result.box[2] = boxes[index * 3 + 2];
                result.score = scores[index];
                result.text = std::move(texts[index]);
                results.push_back(result);
            }
            return results;
        }
        ocr_result recognize(const image &rgb_image) const {
            auto score = 0.0f;
            std::string text;
            recognizer->process(rgb_image, score, text);
            return ocr_result{
                {point{0, 0}, point{rgb_image.width, 0}, point{rgb_image.height, 0}},
                score,
                std::move(text),
            };
        }
    };
} // namespace xx_auto

#include "xx_auto_internal.hpp"
namespace xx_auto // top
{
    const OcrxOnnx *ocr_impl = nullptr;
    void internal::init_ocr(const wchar_t *det_path, const wchar_t *cls_path, const wchar_t *rec_path, const wchar_t *characters_path) {
        if (ocr_impl) delete ocr_impl;
        ocr_impl = new OcrxOnnx(det_path, cls_path, rec_path, characters_path);
    }
    void internal::uninit_ocr() {
        if (ocr_impl) {
            delete ocr_impl;
            ocr_impl = nullptr;
        }
    }
    ocr_result ocr(const image &rgb_image) { return ocr_impl->recognize(rgb_image); }
    std::vector<ocr_result> ocrs(const image &rgb_image, bool rotated) { return ocr_impl->recognizes(rgb_image, rotated); }
} // namespace xx_auto
