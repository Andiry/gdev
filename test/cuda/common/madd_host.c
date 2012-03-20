#include <cuda.h>
#ifdef __KERNEL__ /* just for measurement */
#include <linux/vmalloc.h>
#include <linux/time.h>
#define printf printk
#define malloc vmalloc
#define free vfree
#define gettimeofday(x, y) do_gettimeofday(x)
#else /* just for measurement */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#endif

/* tvsub: ret = x - y. */
static inline void tvsub(struct timeval *x, 
						 struct timeval *y, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec - y->tv_sec;
	ret->tv_usec = x->tv_usec - y->tv_usec;
	if (ret->tv_usec < 0) {
		ret->tv_sec--;
		ret->tv_usec += 1000000;
	}
}

int cuda_test_madd_host(unsigned int n, char *path)
{
	int i, j, idx;
	CUresult res;
	CUdevice dev;
	CUcontext ctx;
	CUfunction function;
	CUmodule module;
	CUdeviceptr a_dev, b_dev, c_dev;
	unsigned int *a_buf, *b_buf, *c_buf;
	int block_x, block_y, grid_x, grid_y;
	char fname[256];
	struct timeval tv;
	struct timeval tv_total_start, tv_total_end;
	float total;
	struct timeval tv_h2d_start, tv_h2d_end;
	float h2d;
	struct timeval tv_d2h_start, tv_d2h_end;
	float d2h;
	struct timeval tv_exec_start, tv_exec_end;
	float exec;

	/* block_x * block_y should not exceed 512. */
	block_x = n < 16 ? n : 16;
	block_y = n < 16 ? n : 16;
	grid_x = n / block_x;
	if (n % block_x != 0)
		grid_x++;
	grid_y = n / block_y;
	if (n % block_y != 0)
		grid_y++;

	gettimeofday(&tv_total_start, NULL);

	res = cuInit(0);
	if (res != CUDA_SUCCESS) {
		printf("cuInit failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuDeviceGet(&dev, 0);
	if (res != CUDA_SUCCESS) {
		printf("cuDeviceGet failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuCtxCreate(&ctx, 0, dev);
	if (res != CUDA_SUCCESS) {
		printf("cuCtxCreate failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	sprintf(fname, "%s/madd_gpu.cubin", path);
	res = cuModuleLoad(&module, fname);
	if (res != CUDA_SUCCESS) {
		printf("cuModuleLoad() failed\n");
		return -1;
	}
	res = cuModuleGetFunction(&function, module, "_Z3addPjS_S_j");
	if (res != CUDA_SUCCESS) {
		printf("cuModuleGetFunction() failed\n");
		return -1;
	}
	res = cuFuncSetSharedSize(function, 0x40); /* just random */
	if (res != CUDA_SUCCESS) {
		printf("cuFuncSetSharedSize() failed\n");
		return -1;
	}
	res = cuFuncSetBlockShape(function, block_x, block_y, 1);
	if (res != CUDA_SUCCESS) {
		printf("cuFuncSetBlockShape() failed\n");
		return -1;
	}

	/* a[] */
	res = cuMemHostAlloc((void **)&a_buf, n*n * sizeof(unsigned int), CU_MEMHOSTALLOC_DEVICEMAP);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostAlloc (a) failed\n");
		return -1;
	}
	res = cuMemHostGetDevicePointer(&a_dev, (void *)a_buf, 0);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostGetDevicePointer (a) failed\n");
		return -1;
	}
	/* b[] */
	res = cuMemHostAlloc((void **)&b_buf, n*n * sizeof(unsigned int), CU_MEMHOSTALLOC_DEVICEMAP);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostAlloc (b) failed\n");
		return -1;
	}
	res = cuMemHostGetDevicePointer(&b_dev, (void *)b_buf, 0);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostGetDevicePointer (b) failed\n");
		return -1;
	}
	/* c[] */
	res = cuMemHostAlloc((void **)&c_buf, n*n * sizeof(unsigned int), CU_MEMHOSTALLOC_DEVICEMAP);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostAlloc (c) failed\n");
		return -1;
	}
	res = cuMemHostGetDevicePointer(&c_dev, (void *)c_buf, 0);
	if (res != CUDA_SUCCESS) {
		printf("cuMemHostGetDevicePointer (c) failed\n");
		return -1;
	}

	gettimeofday(&tv_h2d_start, NULL);
	gettimeofday(&tv_h2d_end, NULL);

	/* initialize A[] & B[] */
	for (i = 0; i < n; i++) {
		for(j = 0; j < n; j++) {
			idx = i * n + j;
			a_buf[idx] = i;
			b_buf[idx] = i + 1;
		}
	}

	/* set kernel parameters */
	res = cuParamSeti(function, 0, a_dev);	
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 4, a_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 8, b_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 12, b_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (b) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 16, c_dev);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 20, c_dev >> 32);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSeti(function, 24, n);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSeti (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuParamSetSize(function, 28);
	if (res != CUDA_SUCCESS) {
		printf("cuParamSetSize failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	gettimeofday(&tv_exec_start, NULL);
	/* launch the kernel */
	res = cuLaunchGrid(function, grid_x, grid_y);
	if (res != CUDA_SUCCESS) {
		printf("cuLaunchGrid failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	cuCtxSynchronize();
	gettimeofday(&tv_exec_end, NULL);

	/* check the results */
	i = j = idx = 0;
	while (i < n) {
		while (j < n) {
			idx = i * n + j;
			if (c_buf[idx] != a_buf[idx] + b_buf[idx]) {
				printf("c[%d] = %d\n", idx, c_buf[idx]);
				printf("a[%d]+b[%d] = %d\n", idx, idx, a_buf[idx]+b_buf[idx]);
			}
			j++;
		}
		i++;
	}

	gettimeofday(&tv_d2h_start, NULL);
	gettimeofday(&tv_d2h_end, NULL);

	res = cuMemFreeHost((void *)a_buf);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFreeHost (a) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuMemFreeHost((void *)b_buf);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFreeHost (b) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	res = cuMemFreeHost((void *)c_buf);
	if (res != CUDA_SUCCESS) {
		printf("cuMemFreeHost (c) failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuModuleUnload(module);
	if (res != CUDA_SUCCESS) {
		printf("cuModuleUnload failed: res = %lu\n", (unsigned long)res);
		return -1;
	}

	res = cuCtxDestroy(ctx);
	if (res != CUDA_SUCCESS) {
		printf("cuCtxDestroy failed: res = %lu\n", (unsigned long)res);
		return -1;
	}
	gettimeofday(&tv_total_end, NULL);

	tvsub(&tv_h2d_end, &tv_h2d_start, &tv);
	h2d = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;
	tvsub(&tv_d2h_end, &tv_d2h_start, &tv);
	d2h = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;
	tvsub(&tv_exec_end, &tv_exec_start, &tv);
	exec = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;
	tvsub(&tv_total_end, &tv_total_start, &tv);
	total = tv.tv_sec * 1000.0 + (float) tv.tv_usec / 1000.0;

	printf("HtoD: %f\n", h2d);
	printf("DtoH: %f\n", d2h);
	printf("Exec: %f\n", exec);
	printf("Time (Memcpy + Launch): %f\n", h2d + d2h + exec);
	printf("Total: %f\n", total);

	return 0;
}

