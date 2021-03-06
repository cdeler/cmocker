//
// Created by cdeler on 10/16/17.
//

#include <sys/mman.h>
#include <zconf.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <assert.h>

#include "vector.h"
#include "cmocker.h"

typedef struct
{
	void *functionAddr;
	size_t originHeader;
	size_t mockedHeader;
	int isMocked;
} FunctionHandle;

static int _change_page_permissions_of_address(void *addr, int perms);
static ssize_t _is_already_mocked(void *function);

static Vector *_functionHandles = NULL;

static void __attribute__((constructor)) __cmocker_used
_cmoker_module_init()
	{
	_functionHandles = vector_open();
	vector_set_deleter(_functionHandles, free);
	}

static void __attribute__((destructor)) __cmocker_used
_cmocker_module_finit()
	{
	vector_close(&_functionHandles);
	}

static ssize_t
_is_already_mocked(void *function)
	{
	size_t i;
	ssize_t result = -1;
	size_t N = vector_getLength(_functionHandles);

	for (i = 0; i < N; ++i)
		{
		FunctionHandle *handle = vector_elementAt(_functionHandles, i);

		if (handle->functionAddr == function)
			{
			assert(i <= SSIZE_MAX);
			result = (ssize_t) i;
			break;
			}
		}

	return result;
	}

static int
_change_page_permissions_of_address(void *addr, int perms)
	{
	// Move the pointer to the page boundary
	size_t page_size = (size_t) getpagesize();
	addr -= (size_t) addr % page_size;

	if (mprotect(addr, page_size, perms) == -1)
		{
		return -1;
		}

	return 0;
	}

#define JMP_OP ((uint8_t)0xE9)

int
cmocker_mock(void *originalFunction, void *mockFunction)
	{
	int rc = -1;

	if (_is_already_mocked(originalFunction) < 0)
		{
		size_t *mockFunctionWords = (size_t *) mockFunction;
		size_t *originalFunctionWords = (size_t *) originalFunction;

		int32_t offset = (int64_t) mockFunctionWords - ((int64_t) originalFunctionWords + 5 * sizeof(char));

		rc = _change_page_permissions_of_address(originalFunction, PROT_READ | PROT_WRITE | PROT_EXEC);

		if (!rc)
			{
			size_t instruction = (size_t) (JMP_OP | offset << 8);
			FunctionHandle *handle = calloc(1, sizeof(FunctionHandle));
			handle->functionAddr = originalFunction;
			handle->mockedHeader = instruction;
			handle->originHeader = *originalFunctionWords;
			handle->isMocked = true;
			vector_pushBack(_functionHandles, handle);

			*originalFunctionWords = instruction;

			rc = 0;
			}
		}

	return rc;
	}

int
cmocker_restore_origin(void *originalFunction)
	{
	ssize_t handleIndex;
	int rc = -1;

	if ((handleIndex = _is_already_mocked(originalFunction)) >= 0)
		{
		FunctionHandle *handle = vector_elementAt(_functionHandles, (size_t) handleIndex);

		handle->isMocked = false;

		size_t *functionWords = handle->functionAddr;
		*functionWords = handle->originHeader;

		handle = vector_removeAt(_functionHandles, (size_t) handleIndex);
		free(handle);
		rc = 0;
		}

	return rc;
	}