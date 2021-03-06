/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <ctime>
#include <fstream>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <likely.h>
#include <likely/opencv.hpp>

using namespace cv;
using namespace llvm;
using namespace std;

#define LIKELY_ERROR_TOLERANCE 0.000001
#define LIKELY_TEST_SECONDS 1

static cl::opt<bool> BenchmarkTest    ("test"     , cl::desc("Run tutorials and unit tests for correctness only"));
static cl::opt<bool> BenchmarkTutorial("tutorial" , cl::desc("Run tutorials instead of unit tests"              ));
static cl::opt<bool> BenchmarkVerbose ("verbose"  , cl::desc("Verbose benchmark output"                         ));
static cl::opt<bool> BenchmarkSerial  ("serial"   , cl::desc("Run serial kernels only"));
static cl::opt<bool> BenchmarkParallel("parallel" , cl::desc("Run parallel kernels only"));
static cl::opt<string> BenchmarkFile    ("file"    , cl::desc("Benchmark the specified file only"    ), cl::value_desc("filename"));
static cl::opt<string> BenchmarkFunction("function", cl::desc("Benchmark the specified function only"), cl::value_desc("string"  ));

static Mat generateData(int rows, int columns, likely_type type, double scaleFactor)
{
    static Mat m;
    if (!m.data) {
        m = imread("../data/misc/lenna.tiff");
        assert(m.data);
        cvtColor(m, m, CV_BGR2GRAY);
    }

    Mat n;
    resize(m, n, Size(columns, rows), 0, 0, INTER_NEAREST);
    n.convertTo(n, likely::typeToDepth(type), scaleFactor);
    return n;
}

struct Test
{
    void run() const
    {
        if (!BenchmarkFunction.empty() && string(function()).compare(0, BenchmarkFunction.size(), BenchmarkFunction))
            return;

        likely_const_ast ast = likely_ast_from_string(function(), false);
        likely_const_env env = likely_new_env_jit();
        likely_const_fun f = likely_compile(ast->atoms[0], env, likely_matrix_void);
        likely_release_env(env);
        likely_release_ast(ast);

        for (likely_type type : types()) {
            for (int size : sizes()) {
                if (BenchmarkTest && (size != 256)) continue;

                // Generate input matrix
                Mat srcCV = generateData(size, size, type, scaleFactor());
                likely_mat srcLikely = fromCvMat(srcCV);

                vector<string> executions;
                executions.push_back("Serial");
                executions.push_back("Parallel");
                for (const string &execution : executions) {
                    likely_set_parallel(&srcLikely->type, execution == "Parallel");
                    if (BenchmarkSerial && likely_parallel(srcLikely->type)) continue;
                    if (BenchmarkParallel && !likely_parallel(srcLikely->type)) continue;
                    likely_mat typeString = likely_type_to_string(type);
                    printf("%s \t%s \t%d \t%s\t", function(), typeString->data, size, execution.c_str());
                    likely_release(typeString);
                    testCorrectness(reinterpret_cast<likely_function_1>(f->function), srcCV, srcLikely);

                    if (BenchmarkTest) {
                        printf("\n");
                        continue;
                    }

                    Speed baseline = testBaselineSpeed(srcCV);
                    Speed likely = testLikelySpeed(reinterpret_cast<likely_function_1>(f->function), srcLikely);
                    printf("%.2e\n", likely.Hz/baseline.Hz);
                }
            }
        }

        likely_release_function(f);
    }

    static void runFile(const string &fileName)
    {
        ifstream file(fileName.compare(fileName.length()-2, 2, ".ll") == 0
                      ? fileName
                      : "../library/" + fileName + ".ll");
        const string source((istreambuf_iterator<char>(file)),
                             istreambuf_iterator<char>());
        if (source.empty()) {
            printf("Failed to read from: %s\n", fileName.c_str());
            return;
        }

        printf("%s \t", fileName.c_str());
        likely_const_ast ast = likely_ast_from_string(source.c_str(), true);
        likely_env env = likely_new_env_jit();
        if (BenchmarkVerbose)
            printf("\n");
        for (size_t i=0; i<ast->num_atoms; i++) {
            if (BenchmarkVerbose) {
                likely_mat str = likely_ast_to_string(ast->atoms[i]);
                printf("%s\n", str->data);
                likely_release(str);
            }
            likely_env new_env = likely_eval(ast->atoms[i], env);
            likely_release_env(env);
            env = new_env;
            if (BenchmarkVerbose && !likely_definition(env->type)) {
                likely_mat str = likely_to_string(env->result, true);
                printf("%s\n\n", str->data);
                likely_release(str);
            }
        }
        likely_release_env(env);

        if (BenchmarkTest) {
            printf("\n");
            return;
        }

        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
            likely_env env = likely_new_env_jit();
            for (size_t i=0; i<ast->num_atoms; i++) {
                likely_env new_env = likely_eval(ast->atoms[i], env);
                likely_release_env(env);
                env = new_env;
            }
            likely_release_env(env);
            endTime = clock();
            iter++;
        }
        Speed speed(iter, startTime, endTime);
        printf("%.2e\n", speed.Hz);
        likely_release_ast(ast);
    }

protected:
    virtual const char *function() const = 0;
    virtual Mat computeBaseline(const Mat &src) const = 0;
    virtual vector<likely_type> types() const
    {
        static vector<likely_type> types;
        if (types.empty()) {
            types.push_back(likely_matrix_u8);
            types.push_back(likely_matrix_u16);
            types.push_back(likely_matrix_i32);
            types.push_back(likely_matrix_f32);
            types.push_back(likely_matrix_f64);
        }
        return types;
    }

    virtual vector<int> sizes() const
    {
        static vector<int> sizes;
        if (sizes.empty()) {
            sizes.push_back(4);
            sizes.push_back(8);
            sizes.push_back(16);
            sizes.push_back(32);
            sizes.push_back(64);
            sizes.push_back(128);
            sizes.push_back(256);
            sizes.push_back(512);
            sizes.push_back(1024);
            sizes.push_back(2048);
            sizes.push_back(4096);
        }
        return sizes;
    }

    virtual bool serial() const { return true; }
    virtual bool parallel() const { return true; }
    virtual double scaleFactor() const { return 1.0; }
    virtual bool ignoreOffByOne() const { return false; } // OpenCV rounds integer division, Likely floors it.

private:
    struct Speed
    {
        int iterations;
        double Hz;
        Speed() : iterations(-1), Hz(-1) {}
        Speed(int iter, clock_t startTime, clock_t endTime)
            : iterations(iter), Hz(double(iter) * CLOCKS_PER_SEC / (endTime-startTime)) {}
    };

    static likely_mat fromCvMat(const Mat &src)
    {
        likely_mat m = likely::fromCvMat(src);
        if (!likely_floating(m->type) && (likely_depth(m->type) <= 16))
            likely_set_saturation(&m->type, true);
        return m;
    }

    void testCorrectness(likely_function_1 f, const Mat &srcCV, const likely_const_mat srcLikely) const
    {
        Mat dstOpenCV = computeBaseline(srcCV);
        likely_const_mat dstLikely = f(srcLikely);

        Mat errorMat = abs(likely::toCvMat(dstLikely) - dstOpenCV);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat = errorMat / (dstOpenCV + LIKELY_ERROR_TOLERANCE); // Normalize errors
        threshold(errorMat, errorMat, LIKELY_ERROR_TOLERANCE, 1, THRESH_BINARY);
        int errors = (int) norm(errorMat, NORM_L1);
        if (errors > 0) {
            likely_const_mat cvLikely = fromCvMat(dstOpenCV);
            stringstream errorLocations;
            errorLocations << "input\topencv\tlikely\trow\tcolumn\n";
            errors = 0;
            for (int i=0; i<srcCV.rows; i++)
                for (int j=0; j<srcCV.cols; j++)
                    if (errorMat.at<float>(i, j) == 1) {
                        const double src = likely_element(srcLikely, 0, j, i, 0);
                        const double cv  = likely_element(cvLikely,  0, j, i, 0);
                        const double dst = likely_element(dstLikely, 0, j, i, 0);
                        if (ignoreOffByOne() && (cv-dst == 1)) continue;
                        if (errors < 100) errorLocations << src << "\t" << cv << "\t" << dst << "\t" << i << "\t" << j << "\n";
                        errors++;
                    }
            if (errors > 0) {
                fprintf(stderr, "Test for: %s differs in: %d location(s):\n%s", function(), errors, errorLocations.str().c_str());
                exit(EXIT_FAILURE);
            }
            likely_release(cvLikely);
        }

        likely_release(dstLikely);
    }

    Speed testBaselineSpeed(const Mat &src) const
    {
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
            computeBaseline(src);
            endTime = clock();
            iter++;
        }
        return Test::Speed(iter, startTime, endTime);
    }

    Speed testLikelySpeed(likely_function_1 f, const likely_const_mat srcLikely) const
    {
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
            likely_const_mat dstLikely = f(srcLikely);
            likely_release(dstLikely);
            endTime = clock();
            iter++;
        }
        return Test::Speed(iter, startTime, endTime);
    }
};

class FloatingTest : public Test
{
    Mat computeBaseline(const Mat &src) const
    {
        if ((src.depth() == CV_32F) || (src.depth() == CV_64F))
            return computeFloatingBaseline(src);
        Mat floating;
        src.convertTo(floating, CV_32F);
        return computeFloatingBaseline(floating);
    }

    virtual Mat computeFloatingBaseline(const Mat &src) const = 0;
};

class ScalarFloatingTest : public FloatingTest
{
    Mat computeFloatingBaseline(const Mat &src) const
    {
        Mat dst(src.rows, src.cols, src.depth());
        const int elements = src.rows * src.cols;
        if (src.depth() == CV_32F) compute32f(reinterpret_cast<const float*>(src.data), reinterpret_cast<float*>(dst.data), elements);
        else                       compute64f(reinterpret_cast<const double*>(src.data), reinterpret_cast<double*>(dst.data), elements);
        return dst;
    }

    virtual void compute32f(const float *src, float *dst, int n) const = 0;
    virtual void compute64f(const double *src, double *dst, int n) const = 0;
};

#define MATH_TEST(FUNC)                                          \
class FUNC##Test : public ScalarFloatingTest {                   \
    const char *function() const { return "a => a." #FUNC ; }    \
    void compute32f(const float *src, float *dst, int n) const   \
        { for (int i=0; i<n; i++) dst[i] = FUNC##f(src[i]); }    \
    void compute64f(const double *src, double *dst, int n) const \
        { for (int i=0; i<n; i++) dst[i] = FUNC(src[i]); }       \
};                                                               \

class addTest : public Test {
    const char *function() const { return "a => a + (a.type 32)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; add(src, 32, dst); return dst; }
};

class subtractTest : public Test {
    const char *function() const { return "a => a - (a.type 32)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; subtract(src, 32, dst); return dst; }
};

class multiplyTest : public Test {
    const char *function() const { return "a => a * (a.type 2)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; multiply(src, 2, dst); return dst; }
};

class divideTest : public Test {
    const char *function() const { return "a => a / (a.type 2)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; divide(src, 2, dst); return dst; }
    bool ignoreOffByOne() const { return true; }
};

class sqrtTest : public FloatingTest {
    const char *function() const { return "a => a.sqrt"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; sqrt(src, dst); return dst; }
};

class powiTest : public FloatingTest {
    const char *function() const { return "a => (powi a 3)"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; pow(src, 3, dst); return dst; }
};

MATH_TEST(sin)
MATH_TEST(cos)

class powTest : public FloatingTest {
    const char *function() const { return "a => (pow a 1.5)"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; pow(src, 1.5, dst); return dst; }
};

MATH_TEST(exp)
MATH_TEST(exp2)

class logTest : public FloatingTest {
    const char *function() const { return "a => a.log"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; log(src, dst); return dst; }
};

MATH_TEST(log10)
MATH_TEST(log2)

class fmaTest : public Test {
    const char *function() const { return "a => a.f * (a.type 2) + (a.type 3)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; src.convertTo(dst, src.depth() == CV_64F ? CV_64F : CV_32F, 2, 3); return dst; }
};

class copysignTest : public Test {
    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_matrix_f32);
        types.push_back(likely_matrix_f64);
        return types;
    }
    const char *function() const { return "a => (a.type (copysign a -1))"; }
    Mat computeBaseline(const Mat &src) const
    {
        Mat dst(src.rows, src.cols, src.depth());
        const int elements = src.rows * src.cols;
        if (src.depth() == CV_32F) compute32f(reinterpret_cast<const float*>(src.data), reinterpret_cast<float*>(dst.data), elements);
        else                       compute64f(reinterpret_cast<const double*>(src.data), reinterpret_cast<double*>(dst.data), elements);
        return dst;
    }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = copysignf(src[i], -1); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = copysign(src[i], -1); }
};

MATH_TEST(floor)
MATH_TEST(ceil)
MATH_TEST(trunc)
MATH_TEST(rint)
MATH_TEST(nearbyint)
MATH_TEST(round)

class castTest : public Test {
    const char *function() const { return "a => a.f32"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; src.convertTo(dst, CV_32F); return dst; }
};

class thresholdTest : public Test {
    const char *function() const { return "a => (a.type a > 127)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; threshold(src, dst, 127, 1, THRESH_BINARY); return dst; }
    vector<likely_type> types() const { vector<likely_type> types; types.push_back(likely_matrix_u8); types.push_back(likely_matrix_f32); return types; }
};

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    // Print to console immediately
    setbuf(stdout, NULL);

    if (!BenchmarkFile.empty()) {
        Test::runFile(BenchmarkFile);
        return EXIT_SUCCESS;
    }

    if (BenchmarkTutorial || BenchmarkTest) {
        printf("File     \tSpeed (Hz)\n");
        ifstream file("../library/tutorial.ll");
        string line;
        // Skip header
        getline(file, line);
        getline(file, line);
        while (getline(file, line)) {
            string::size_type index = line.find("=", 0);
            Test::runFile(line.substr(index+1, line.size()-index-2));
        }
    }

    if (!BenchmarkTutorial || BenchmarkTest) {
        printf("Function \tType \tSize \tExecution \tSpeedup\n");
        addTest().run();
        subtractTest().run();
        multiplyTest().run();
        divideTest().run();
        sqrtTest().run();
        powiTest().run();
        sinTest().run();
        cosTest().run();
        powTest().run();
        expTest().run();
        exp2Test().run();
        logTest().run();
        log10Test().run();
        log2Test().run();
        fmaTest().run();
        copysignTest().run();
        floorTest().run();
        ceilTest().run();
        truncTest().run();
        rintTest().run();
        nearbyintTest().run();
        roundTest().run();
        castTest().run();
        thresholdTest().run();
    }

    return EXIT_SUCCESS;
}

