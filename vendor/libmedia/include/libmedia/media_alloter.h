#ifndef __MEDIA_PMEM_MANAGER_H__
#define __MEDIA_PMEM_MANAGER_H__
#include <libutils2/list.h>

#include <pthread.h>
#include <libmedia/video_frame.h>

struct media_alloter{
    struct list_head free_list;
    struct list_head alloc_list;
    void *mmap_mem;
    unsigned long phys_mem;
    int size;
    pthread_mutex_t lock;
};


struct media_alloter* media_alloter_open(void *mmap_mem, unsigned long phys_mem, int size);

void *media_alloter_alloc(struct media_alloter *alloter, unsigned long *phys_mem, int size);

struct video_frame *media_alloter_alloc_video_frame(struct media_alloter *alloter, int width, int height,
                                                    enum video_frame_format format, int bytes_align);

void media_alloter_free(struct media_alloter *alloter, void *mem, unsigned long phys_mem, int size);

void media_alloter_close(struct media_alloter *alloter);


struct video_frame *media_alloter_alloc_video_frame_planar(struct media_alloter *alloter, int width, int height,
                                                           enum video_frame_format format, int bytes_align);


#endif