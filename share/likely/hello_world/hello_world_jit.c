#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <likely.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char *input_image, *filter, *output_image;

    if (argc == 1) {
        input_image = "../data/misc/lenna.tiff"; // Assume we are run from a hypothetical <root>/bin folder
        output_image = "";
        filter = "a => a / (a.type 2)";
    } else if (argc == 4) {
        input_image = argv[1];
        output_image = argv[3];

        FILE* fp = fopen(argv[2], "rb");
        if (!fp) {
            printf("Failed to open filter!\n");
            return -1;
        }

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        filter = malloc(size);
        long size_read = (long)fread(filter, 1, size, fp);
        if (size_read != size) {
            printf("Failed to read filter!\n");
            return -1;
        }
        filter[size] = 0;
    } else {
        printf("Usage:\n");
        printf("\thello_world_jit\n");
        printf("\thello_world_jit <input_image> <filter_file> <output_image>\n");
        return -1;
    }

    printf("Reading input image...\n");
    likely_const_mat lenna = likely_read(input_image, likely_file_binary);
    if (lenna) {
        printf("Width: %d\nHeight: %d\n", (int) lenna->columns, (int) lenna->rows);
    } else {
        printf("Failed to read!\n");
        return -1;
    }

    if (lenna->rows == 0 || lenna->columns == 0) {
        printf("Image width or height is zero!\n");
        return -1;
    }

    printf("Parsing abstract syntax tree...\n");
    likely_const_ast ast = likely_ast_from_string(filter, false);

    printf("Creating a compiler environment...\n");
    likely_const_env env = likely_new_env_jit();

    printf("Compiling source code...\n");
    likely_const_fun darken = likely_compile(ast->atoms[0], env, likely_matrix_void);
    if (!darken->function) {
        printf("Failed to compile!\n");
        return -1;
    }

    printf("Calling compiled function...\n");
    likely_const_mat dark_lenna = ((likely_function_1)darken->function)(lenna);
    if (!dark_lenna) {
        printf("Failed to execute!\n");
        return -1;
    }

    if (strcmp(output_image, "")) {
        printf("Writing output image...\n");
        likely_write(dark_lenna, output_image);
    }

    printf("Cleaning up...\n");
    likely_release(dark_lenna);
    likely_release(lenna);
    likely_release_ast(ast);
    likely_release_env(env);
    likely_release_function(darken);

    printf("Done!\n");
    return 0;
}
