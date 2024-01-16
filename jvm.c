#include "jvm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "read_class.h"

/** The name of the method to invoke to run the class file */
const char MAIN_METHOD[] = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char MAIN_DESCRIPTOR[] = "([Ljava/lang/String;)V";

/**
 * Represents the return value of a Java method: either void or an int or a reference.
 * For simplification, we represent a reference as an index into a heap-allocated array.
 * (In a real JVM, methods could also return object references or other primitives.)
 */
typedef struct {
    /** Whether this returned value is an int */
    bool has_value;
    /** The returned value (only valid if `has_value` is true) */
    int32_t value;
} optional_value_t;

/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @param heap an array of heap-allocated pointers, useful for references
 * @return an optional int containing the method's return value
 */
optional_value_t execute(method_t *method, int32_t *locals, class_file_t *class,
                         heap_t *heap) {
    /* You should remove these casts to void in your solution.
     * They are just here so the code compiles without warnings. */
    (void) class;
    (void) heap;

    size_t pc = 0;
    int s_i = 0;
    u1 *code_inst = method->code.code;
    int32_t *stack = calloc(method->code.max_stack, sizeof(int32_t));
    u1 curr = code_inst[pc];
    optional_value_t result = {.has_value = false};
    while (curr != i_return) {
        curr = code_inst[pc];
        // printf("%d\n", curr);
        if (curr == i_bipush) {
            stack[s_i] = (int32_t)((int8_t) code_inst[pc + 1]);
            pc += 2;
            s_i++;
        }
        else if (curr == i_iadd) {
            int32_t sum = stack[s_i - 2] + stack[s_i - 1];
            stack[s_i - 2] = sum;
            s_i--;
            pc++;
        }
        else if (curr == i_getstatic) {
            pc += 3;
        }
        else if (curr == i_invokevirtual) {
            printf("%d\n", stack[s_i - 1]);
            s_i--;
            pc += 3;
        }
        else if (curr >= i_iconst_m1 && curr <= i_iconst_5) {
            stack[s_i] = (int8_t) curr - i_iconst_0;
            s_i++;
            pc++;
        }
        else if (curr == i_sipush) {
            stack[s_i] = (short) ((code_inst[pc + 1] << 8) | code_inst[pc + 2]);
            // printf("%d\n", stack[s_i]);
            s_i++;
            pc += 3;
        }
        else if (curr == i_isub) {
            int32_t diff = stack[s_i - 2] - stack[s_i - 1];
            stack[s_i - 2] = diff;
            s_i--;
            pc++;
        }
        else if (curr == i_imul) {
            int32_t product = stack[s_i - 2] * stack[s_i - 1];
            stack[s_i - 2] = product;
            s_i--;
            pc++;
        }
        else if (curr == i_idiv && stack[s_i - 1] != 0) {
            int32_t quotient = stack[s_i - 2] / stack[s_i - 1];
            stack[s_i - 2] = quotient;
            s_i--;
            pc++;
        }
        else if (curr == i_irem && stack[s_i - 1] != 0) {
            int32_t remainder = stack[s_i - 2] % stack[s_i - 1];
            stack[s_i - 2] = remainder;
            s_i--;
            pc++;
        }
        else if (curr == i_ineg) {
            int32_t val = stack[s_i - 1];
            stack[s_i - 1] = (int32_t) -1 * val;
            pc++;
        }
        else if (curr == i_ishl) {
            int32_t left_shift = stack[s_i - 2] << stack[s_i - 1];
            stack[s_i - 2] = left_shift;
            s_i--;
            pc++;
        }
        else if (curr == i_ishr) {
            int32_t right_shift = stack[s_i - 2] >> stack[s_i - 1];
            stack[s_i - 2] = right_shift;
            s_i--;
            pc++;
        }
        else if (curr == i_iushr) {
            uint32_t u_right_shift = ((uint32_t) stack[s_i - 2]) >> stack[s_i - 1];
            stack[s_i - 2] = u_right_shift;
            s_i--;
            pc++;
        }
        else if (curr == i_iand) {
            int32_t and = stack[s_i - 2] & stack[s_i - 1];
            stack[s_i - 2] = and;
            s_i--;
            pc++;
        }
        else if (curr == i_ior) {
            int32_t or = stack[s_i - 2] | stack[s_i - 1];
            stack[s_i - 2] = or ;
            s_i--;
            pc++;
        }
        else if (curr == i_ixor) {
            int32_t xor = stack[s_i - 2] ^ stack[s_i - 1];
            stack[s_i - 2] = xor;
            s_i--;
            pc++;
        }
        else if (curr == i_iload || curr == i_aload) {
            stack[s_i] = locals[code_inst[pc + 1]];
            s_i++;
            pc += 2;
        }
        else if (curr == i_istore || curr == i_astore) {
            locals[code_inst[pc + 1]] = stack[s_i - 1];
            s_i--;
            pc += 2;
        }
        else if (curr == i_iinc) {
            locals[code_inst[pc + 1]] += (int8_t) code_inst[pc + 2];
            pc += 3;
        }
        else if (curr <= i_iload_3 && curr >= i_iload_0) {
            stack[s_i] = locals[curr - i_iload_0];
            s_i++;
            pc++;
        }
        else if (curr <= i_istore_3 && curr >= i_istore_0) {
            locals[curr - i_istore_0] = stack[s_i - 1];
            s_i--;
            pc++;
        }
        else if (curr == i_ldc) {
            stack[s_i] =
                ((CONSTANT_Integer_info *) class->constant_pool[code_inst[pc + 1] - 1]
                     .info)
                    ->bytes;
            s_i++;
            pc += 2;
        }
        else if (curr >= i_ifeq && curr <= i_ifle) {
            pc = jump_one(curr, code_inst, stack, pc, s_i);
            s_i--;
        }
        else if (curr >= i_if_icmpeq && curr <= i_if_icmple) {
            pc = jump_two(curr, code_inst, stack, pc, s_i);
            s_i -= 2;
        }
        else if (curr == i_goto) {
            pc += (short) ((code_inst[pc + 1] << 8) | code_inst[pc + 2]);
        }
        else if (curr == i_ireturn || curr == i_areturn) {
            result.has_value = true;
            result.value = stack[s_i - 1];
            s_i--;
            pc++;
            break;
        }
        else if (curr == i_invokestatic) {
            u2 idx = (u2)((code_inst[pc + 1] << 8) | code_inst[pc + 2]);
            method_t *child = find_method_from_index(idx, class);
            int32_t *locals_c = calloc(child->code.max_locals, sizeof(int32_t));
            for (int32_t i = get_number_of_parameters(child) - 1; i >= 0; i--) {
                locals_c[i] = stack[s_i - 1];
                s_i--;
            }
            optional_value_t res = execute(child, locals_c, class, heap);
            free(locals_c);
            if (res.has_value) {
                stack[s_i] = res.value;
                s_i++;
            }
            pc += 3;
        }
        else if (curr == i_nop) {
            pc++;
        }
        else if (curr == i_dup) {
            stack[s_i] = stack[s_i - 1];
            s_i++;
            pc++;
        }
        else if (curr == i_newarray) {
            int32_t *arr = calloc(stack[s_i - 1] + 1, sizeof(int32_t));
            arr[0] = stack[s_i - 1];
            int32_t ref = heap_add(heap, arr);
            stack[s_i - 1] = ref;
            pc += 2;
        }
        else if (curr == i_arraylength) {
            int32_t *arr = heap_get(heap, stack[s_i - 1]);
            stack[s_i - 1] = arr[0];
            pc++;
        }
        else if (curr == i_iastore) {
            int32_t *arr = heap_get(heap, stack[s_i - 3]);
            arr[stack[s_i - 2] + 1] = stack[s_i - 1];
            s_i -= 3;
            pc++;
        }
        else if (curr == i_iaload) {
            int32_t *arr = heap_get(heap, stack[s_i - 2]);
            stack[s_i - 2] = arr[stack[s_i - 1] + 1];
            s_i--;
            pc++;
        }
        // else if (curr == i_aload) {
        //     stack[s_i] = locals[code_inst[pc + 1]];
        //     s_i++;
        //     pc += 2;
        // }
        else if (curr <= i_aload_3 && curr >= i_aload_0) {
            stack[s_i] = locals[curr - i_aload_0];
            s_i++;
            pc++;
        }
        else if (curr <= i_astore_3 && curr >= i_astore_0) {
            locals[curr - i_astore_0] = stack[s_i - 1];
            s_i--;
            pc++;
        }
    }
    free(stack);

    return result;
}

size_t jump_one(u1 curr, const u1 *code_inst, const int32_t *stack, size_t pc_prev,
                int s_i) {
    int32_t a = stack[s_i - 1];
    size_t pc = pc_prev;

    bool b_ifeq = (curr == i_ifeq);
    bool b_ifne = (curr == i_ifne);
    bool b_iflt = (curr == i_iflt);
    bool b_ifge = (curr == i_ifge);
    bool b_ifgt = (curr == i_ifgt);
    bool b_ifle = (curr == i_ifle);

    if ((b_ifeq && a == 0) || (b_ifne && a != 0) || (b_iflt && a < 0) ||
        (b_ifge && a >= 0) || (b_ifgt && a > 0) || (b_ifle && a <= 0)) {
        pc += (short) ((code_inst[pc + 1] << 8) | code_inst[pc + 2]);
        return pc;
    }
    pc += 3;
    return pc;
}

size_t jump_two(u1 curr, const u1 *code_inst, const int32_t *stack, size_t pc_prev,
                int s_i) {
    int32_t a = stack[s_i - 2];
    int32_t b = stack[s_i - 1];
    size_t pc = pc_prev;

    bool b_if_icmpeq = (curr == i_if_icmpeq);
    bool b_if_icmpne = (curr == i_if_icmpne);
    bool b_if_icmplt = (curr == i_if_icmplt);
    bool b_if_icmpge = (curr == i_if_icmpge);
    bool b_if_icmpgt = (curr == i_if_icmpgt);
    bool b_if_icmple = (curr == i_if_icmple);

    if ((b_if_icmpeq && a == b) || (b_if_icmpne && a != b) || (b_if_icmplt && a < b) ||
        (b_if_icmpge && a >= b) || (b_if_icmpgt && a > b) || (b_if_icmple && a <= b)) {
        pc += (short) ((code_inst[pc + 1] << 8) | code_inst[pc + 2]);
        return pc;
    }
    pc += 3;
    return pc;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file != NULL && "Failed to open file");

    // Parse the class file
    class_file_t *class = get_class(class_file);
    int error = fclose(class_file);
    assert(error == 0 && "Failed to close file");

    // The heap array is initially allocated to hold zero elements.
    heap_t *heap = heap_init();

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, class);
    assert(main_method != NULL && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];
    // Initialize all local variables to 0
    memset(locals, 0, sizeof(locals));
    optional_value_t result = execute(main_method, locals, class, heap);
    assert(!result.has_value && "main() should return void");

    // Free the internal data structures
    free_class(class);

    // Free the heap
    heap_free(heap);
}
