
static int leading_bit_cnt(unsigned char c)
{
    int cnt = 0;
    for (int i = 7; i >= 0; i--, cnt++) {
        if (!(c & (1 << i)))
            return cnt;
    }

    return cnt;
}

static int check_cnt_valid(const char *str, int cnt)
{
    for (int i = 0; i < cnt; i++) {
        unsigned char c = str[i];
        if (c == 0)
            return -1;
        if ((c >> 6) != 0b10)
            return -1;
    }
    return 0;
}

static int to_unicode(const char *str, int cnt)
{
    unsigned int msk = (1 << (8-cnt)) - 1;
    const unsigned char *p = (const unsigned char *)str;

    unsigned int code = (p[0] & msk) << ((cnt-1)*6);

    for (int i = 1; i < cnt; i++) {
        code |= (p[i] & 0b111111) << ((cnt-i-1)*6);
    }

    return code;
}

int utf8_to_unicode(const char *str, char **endp)
{
    const char *p = str;
    int unicode = 0;

    unsigned char c = p[0];
    if (c == 0) {
        unicode = 0;
        goto out;
    }

    int cnt = leading_bit_cnt(c);
    if (cnt == 0) {
        unicode = c;
        p++;
        goto out;
    }

    if (cnt > 6 || cnt == 1) {
        unicode = '!';
        p++;
        goto out;
    }

    if (check_cnt_valid(p+1, cnt-1)) {
        unicode = '@';
        p++;
        goto out;
    }

    unicode = to_unicode(p, cnt);
    p += cnt;

out:
    if (endp)
        *endp = (void *)p;
    return unicode;
}
