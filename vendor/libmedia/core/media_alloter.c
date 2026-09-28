#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <libmedia/video_frame.h>

#include <libmedia/media_alloter.h>

#ifdef APP_libmedia_rmem
#include <libhardware2/rmem.h>
static int rmem_fd = -1;

void *media_alloter_rmem_alloc(unsigned long *phys_mem, int size)
{
    if(rmem_fd < 0) {
        rmem_fd = rmem_open();
        if(rmem_fd < 0) {
            fprintf(stderr, "media_alloter: faile to open rmem\n");
            return NULL;
        }
    }

    return rmem_alloc(rmem_fd, phys_mem, size);
}

void media_alloter_rmem_free(void *mem, unsigned long phys_mem, int size)
{
    if (rmem_fd < 0) {
        rmem_fd = rmem_open();
        if(rmem_fd < 0) {
            fprintf(stderr, "media_alloter: faile to open rmem\n");
            return;
        }
    }

    rmem_free(rmem_fd, mem, phys_mem, size);
}
#else
void *media_alloter_rmem_alloc(unsigned long *phys_mem, int size)
{
    fprintf(stderr, "error: rmem not enable\n");
    return NULL;
}

void media_alloter_rmem_free(void *mem, unsigned long phys_mem, int size)
{
    return;
}
#endif

#define DEFAULT_BYTES_ALIGN       8

struct pmem {
    struct list_head link;
    unsigned long alloc_phys;
    void *alloc_mem;
    int size;
};

static void pmem_add_free(struct media_alloter *alloter, struct pmem *pmem)
{
    struct pmem *temp;
    struct list_head *pos;

    list_for_each(pos, &alloter->free_list) {
        temp = list_entry(pos, struct pmem, link);

        /*找到rmem 后面的第一个节点（temp）*/
        if (temp->alloc_phys > pmem->alloc_phys) {

            struct pmem *temp_prev = list_entry(temp->link.prev, struct pmem, link);

            /*先判断rmem ---- temp是否相连*/
            if (pmem->alloc_phys + pmem->size == temp->alloc_phys) {
                list_del(&temp->link);
                pmem->size += temp->size;
                free(temp);
            }

            /*判断temp前一个节点是否为头，是：添加到头后面就退出，否继续*/
            if (!temp_prev) {
                list_add(&pmem->link, &alloter->free_list);
                return;
            }

            /*判断temp_prev ----- rmem是否相连*/
            if (temp_prev->alloc_phys + temp_prev->size == pmem->alloc_phys) {
                temp_prev->size += pmem->size;
                free(pmem);
            } else {
                list_add(&pmem->link, &temp_prev->link);
            }

            return;
        }

        /*pmem 后面没有节点了*/
        if (list_is_last(pos, &alloter->free_list)) {
            if (temp->alloc_phys + temp->size == pmem->alloc_phys) {
                temp->size += pmem->size;
                free(pmem);
                return;
            }
        }
    }

    list_add_tail(&pmem->link, &alloter->free_list);
}


struct media_alloter* media_alloter_open(void *mmap_mem, unsigned long phys_mem, int size)
{
    struct media_alloter *alloter = malloc(sizeof(*alloter));
    assert(alloter);

    INIT_LIST_HEAD(&alloter->alloc_list);
    INIT_LIST_HEAD(&alloter->free_list);

    alloter->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    struct pmem *pmem = malloc(sizeof(*pmem));
    assert(pmem);

    pmem->alloc_mem = mmap_mem;
    pmem->alloc_phys = phys_mem;
    pmem->size = size;

    list_add_tail(&pmem->link, &alloter->free_list);

    alloter->mmap_mem = mmap_mem;
    alloter->phys_mem = phys_mem;
    alloter->size = size;

    return alloter;
}

void *media_alloter_alloc(struct media_alloter *alloter, unsigned long *phys_mem, int size)
{
    int is_found = 0;
    void *ret;
    struct pmem *pmem;
    int need_size = size;

    if (!alloter)
        return media_alloter_rmem_alloc(phys_mem, size);

    pthread_mutex_lock(&alloter->lock);

    list_for_each_entry(pmem, &alloter->free_list, link) {
        if(pmem->size >= need_size) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        ret = media_alloter_rmem_alloc(phys_mem, size);
        goto unlock;
    }

    if (pmem->size - need_size == 0 || pmem->size - need_size < 4) {
        list_del(&pmem->link);
        list_add_tail(&pmem->link, &alloter->alloc_list);
        ret = (void *)pmem->alloc_mem;
        *phys_mem = pmem->alloc_phys;
        goto unlock;
    }

    struct pmem *pmem_new = malloc(sizeof(*pmem_new));
    assert(pmem_new);

    list_del(&pmem->link);

    pmem_new->alloc_mem = pmem->alloc_mem + need_size;
    pmem_new->alloc_phys = pmem->alloc_phys + need_size;
    pmem_new->size = pmem->size - need_size;
    pmem_add_free(alloter, pmem_new);

    pmem->size = need_size;

    ret = pmem->alloc_mem;
    *phys_mem = pmem->alloc_phys;
    list_add_tail(&pmem->link, &alloter->alloc_list);

unlock:
    pthread_mutex_unlock(&alloter->lock);
    return ret;

}

void media_alloter_free(struct media_alloter *alloter, void *mem, unsigned long phys_mem, int size)
{

    int is_found = 0;
    struct pmem *alloc;

    if (!alloter) {
        media_alloter_rmem_free(mem, phys_mem, size);
        return;
    }


    pthread_mutex_lock(&alloter->lock);

    list_for_each_entry(alloc, &alloter->alloc_list, link) {
        if (alloc->alloc_phys == phys_mem) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        media_alloter_rmem_free(mem, phys_mem, size);
        pthread_mutex_unlock(&alloter->lock);
        return;
    }

    list_del(&alloc->link);

    pmem_add_free(alloter, alloc);

    pthread_mutex_unlock(&alloter->lock);
}

void media_alloter_close(struct media_alloter *alloter)
{
    struct pmem *first = list_first_entry(&alloter->free_list, struct pmem, link);

    assert(first->alloc_mem == alloter->mmap_mem && first->size == alloter->size);

    free(first);
    free(alloter);
}

static void media_alloter_free_frame(void *handle, struct video_frame *frame)
{
    struct media_alloter *alloter = handle;
    int total_size = frame->total_size;

    media_alloter_free(alloter, frame->data[0], frame->phys_data[0], total_size);

    video_frame_free(frame);
}

struct video_frame *media_alloter_alloc_video_frame(struct media_alloter *alloter, int width, int height,
                                                    enum video_frame_format format, int bytes_align)
{
    int ret;
    struct video_frame *frame = video_frame_alloc();
    assert(frame);

    void *mem;
    unsigned long phys_mem;

    int video_size = video_format_size(format, width, height, bytes_align);

    mem = media_alloter_alloc(alloter, &phys_mem, video_size);
    if(!mem) {
        fprintf(stderr, "media_alloter: failed to alloc frame\n");
        return NULL;
    }

    ret = video_frame_init(frame, width, height, format, bytes_align, mem,
                               phys_mem, alloter, media_alloter_free_frame);

    if (ret < 0)
        return NULL;

    video_frame_get(frame);

    return frame;
}


static void media_alloter_free_frame_planar(void *handle, struct video_frame *frame)
{
    struct media_alloter *alloter = handle;

    int i;
    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        if (frame->data[i])
            media_alloter_free(alloter, frame->data[i], frame->phys_data[i], frame->size[i]);
    }

    video_frame_free(frame);
}


struct video_frame *media_alloter_alloc_video_frame_planar(struct media_alloter *alloter, int width, int height,
                                                    enum video_frame_format format, int bytes_align)

{
    struct video_frame *frame = video_frame_alloc();
    assert(frame);

    void *mem;
    unsigned long phys_mem;


    frame->width = width;
    frame->height = height;
    frame->format = format;
    frame->total_size = 0;

    int i;
    for (i = 0; i < MAX_VIDEO_FRAME_BUF_CNT; i++) {
        int size = video_format_planar_size(format, i, width, height, bytes_align);
        if (!size)
            continue;

        mem = media_alloter_alloc(alloter, &phys_mem, size);
        if (!mem) {
            fprintf(stderr, "media_alloter: failed to alloc frame\n");
            media_alloter_free_frame_planar(alloter, frame);
            return NULL;
        }

        frame->data[i] = size ? mem : NULL;
        frame->size[i] = size;
        frame->linesize[i] = video_format_line_size(format, i, width, bytes_align);
        frame->total_size += size;
        frame->phys_data[i] = size ? phys_mem : 0;
    }

    frame->is_phys = 1;
    frame->user_cnt = 0;

    frame->handle = (void *)alloter;
    frame->put_frame = media_alloter_free_frame_planar;


    video_frame_get(frame);

    return frame;
}
