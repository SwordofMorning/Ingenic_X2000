#include <stdlib.h>
#include <string.h>
#include <libmedia/media_packet.h>


void default_media_packet_put(void *handle, struct media_packet *pkt)
{
    media_packet_free(pkt);
}

struct media_packet *media_packet_alloc(void)
{
    struct media_packet *packet = malloc(sizeof(*packet));
    if (!packet) {
        fprintf(stderr, "failed to alloc packet\n");
        return NULL;
    }

    memset(packet, 0, sizeof(*packet));

    packet->handle = NULL;
    packet->put_packet = default_media_packet_put;

    return packet;
}

void media_packet_free(struct media_packet *packet)
{
    if(!packet)
        return;

    assert(!packet->ref_cnt);
    free(packet);
}

void media_packet_get(struct media_packet *packet)
{
    packet->ref_cnt++;
}

void media_packet_put(struct media_packet *packet)
{
    if(!packet)
        return;

    packet->ref_cnt--;

    assert(packet->ref_cnt >= 0);

    if (!packet->ref_cnt && packet->put_packet)
        packet->put_packet(packet->handle, packet);

}