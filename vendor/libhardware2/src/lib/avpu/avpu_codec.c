/*
 * 调用 libavpu.so中的接口
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>


int AL_Codec_Create(void);
void AL_Codec_Destroy(void);
void *IMP_Encoder_VbmAlloc(uint32_t size, uint32_t align);
void IMP_Encoder_VbmFree(void *vaddr);
intptr_t IMP_Encoder_VbmV2P(intptr_t vaddr);
intptr_t IMP_Encoder_VbmP2V(intptr_t paddr);

/*******************************************************************/

int alloc_rmem_get_size(void)
{
    return 16 *1024 *1024;
}

int avpu_encoder_init(void)
{
    return AL_Codec_Create();
}

void avpu_encoder_deinit(void)
{
    AL_Codec_Destroy();
}

void *avpu_encoder_alloc(int size)
{
    return IMP_Encoder_VbmAlloc(size, 256);
}

void avpu_encoder_free(void *vaddr)
{
    IMP_Encoder_VbmFree(vaddr);
}

void *avpu_encoder_vitr_to_phys(void *vaddr)
{
    return (void *)IMP_Encoder_VbmV2P((intptr_t)vaddr);
}

void *avpu_encoder_phys_to_virt(void *paddr)
{
    return (void *)IMP_Encoder_VbmP2V((intptr_t)paddr);
}