// clang -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -lssl -lcrypto -O0 -g aesni_cbc_hmac_sha1_cipher.c && ./a.out

#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#define NO_PAYLOAD_LENGTH       ((size_t)-1)

void sha1_block_data_order(void *c, const void *p, size_t len);


static int aesni_cbc_hmac_sha1_cipher(unsigned char *out, size_t len)
{
    SHA_CTX md;
    SHA1_Init(&md);
    SHA_CTX md_head;
    SHA1_Init(&md_head);
    SHA_CTX md_tail;
    SHA1_Init(&md_tail);

    unsigned int l;
    // size_t plen = key->payload_length, iv = 0, /* explicit IV in TLS 1.1 and
    //                                             * later */
    size_t sha_off = 0;
    size_t aes_off = 0, blocks;

    sha_off = SHA_CBLOCK - md.num;

    // key->payload_length = NO_PAYLOAD_LENGTH;

    if (len % AES_BLOCK_SIZE)
        return 0;

    union {
        unsigned int u[SHA_DIGEST_LENGTH / sizeof(unsigned int)];
        unsigned char c[32 + SHA_DIGEST_LENGTH];
    } mac, *pmac;

    /* arrange cache line alignment */
    pmac = (void *)(((size_t)mac.c + 31) & ((size_t)0 - 32));

    size_t inp_len, mask, j, i;
    unsigned int res, maxpad, pad, bitlen;
    int ret = 1;
    union {
        unsigned int u[SHA_LBLOCK];
        unsigned char c[SHA_CBLOCK];
    } *data = (void *)md.data;

    /* figure out payload length */
    pad = out[len - 1];
    maxpad = len - (SHA_DIGEST_LENGTH + 1);
    maxpad |= (255 - maxpad) >> (sizeof(maxpad) * 8 - 8);
    maxpad &= 255;

    printf("maxpad, pad: %d %d\n", maxpad, pad);

    inp_len = len - (SHA_DIGEST_LENGTH + pad + 1);
    mask = (0 - ((inp_len - len) >> (sizeof(inp_len) * 8 - 1)));
    inp_len &= mask;
    ret &= (int)mask;

    printf("ret (inp_len): %d\n", ret);

    // key->aux.tls_aad[plen - 2] = inp_len >> 8;
    // key->aux.tls_aad[plen - 1] = inp_len;

    /* calculate HMAC */
    md = md_head;
    // SHA1_Update(&md, key->aux.tls_aad, plen);

    len -= SHA_DIGEST_LENGTH; /* amend mac */
    if (len >= (256 + SHA_CBLOCK)) {
        j = (len - (256 + SHA_CBLOCK)) & (0 - SHA_CBLOCK);
        j += SHA_CBLOCK - md.num;
        SHA1_Update(&md, out, j);
        out += j;
        len -= j;
        inp_len -= j;
    }

    /* but pretend as if we hashed padded payload */
    bitlen = md.Nl + (inp_len << 3); /* at most 18 bits */
#  ifdef BSWAP4
    bitlen = BSWAP4(bitlen);
#  else
    mac.c[0] = 0;
    mac.c[1] = (unsigned char)(bitlen >> 16);
    mac.c[2] = (unsigned char)(bitlen >> 8);
    mac.c[3] = (unsigned char)bitlen;
    bitlen = mac.u[0];
#  endif

    pmac->u[0] = 0;
    pmac->u[1] = 0;
    pmac->u[2] = 0;
    pmac->u[3] = 0;
    pmac->u[4] = 0;

    for (res = md.num, j = 0; j < len; j++) {
        size_t c = out[j];
        mask = (j - inp_len) >> (sizeof(j) * 8 - 8);
        c &= mask;
        c |= 0x80 & ~mask & ~((inp_len - j) >> (sizeof(j) * 8 - 8));
        data->c[res++] = (unsigned char)c;

        if (res != SHA_CBLOCK)
            continue;

        /* j is not incremented yet */
        mask = 0 - ((inp_len + 7 - j) >> (sizeof(j) * 8 - 1));
        data->u[SHA_LBLOCK - 1] |= bitlen & mask;
        sha1_block_data_order(&md, data, 1);
        mask &= 0 - ((j - inp_len - 72) >> (sizeof(j) * 8 - 1));
        pmac->u[0] |= md.h0 & mask;
        pmac->u[1] |= md.h1 & mask;
        pmac->u[2] |= md.h2 & mask;
        pmac->u[3] |= md.h3 & mask;
        pmac->u[4] |= md.h4 & mask;
        res = 0;
    }

    for (i = res; i < SHA_CBLOCK; i++, j++)
        data->c[i] = 0;

    if (res > SHA_CBLOCK - 8) {
        mask = 0 - ((inp_len + 8 - j) >> (sizeof(j) * 8 - 1));
        data->u[SHA_LBLOCK - 1] |= bitlen & mask;
        sha1_block_data_order(&md, data, 1);
        mask &= 0 - ((j - inp_len - 73) >> (sizeof(j) * 8 - 1));
        pmac->u[0] |= md.h0 & mask;
        pmac->u[1] |= md.h1 & mask;
        pmac->u[2] |= md.h2 & mask;
        pmac->u[3] |= md.h3 & mask;
        pmac->u[4] |= md.h4 & mask;

        memset(data, 0, SHA_CBLOCK);
        j += 64;
    }
    data->u[SHA_LBLOCK - 1] = bitlen;
    sha1_block_data_order(&md, data, 1);
    mask = 0 - ((j - inp_len - 73) >> (sizeof(j) * 8 - 1));
    pmac->u[0] |= md.h0 & mask;
    pmac->u[1] |= md.h1 & mask;
    pmac->u[2] |= md.h2 & mask;
    pmac->u[3] |= md.h3 & mask;
    pmac->u[4] |= md.h4 & mask;

#  ifdef BSWAP4
    pmac->u[0] = BSWAP4(pmac->u[0]);
    pmac->u[1] = BSWAP4(pmac->u[1]);
    pmac->u[2] = BSWAP4(pmac->u[2]);
    pmac->u[3] = BSWAP4(pmac->u[3]);
    pmac->u[4] = BSWAP4(pmac->u[4]);
#  else
    for (i = 0; i < 5; i++) {
        res = pmac->u[i];
        pmac->c[4 * i + 0] = (unsigned char)(res >> 24);
        pmac->c[4 * i + 1] = (unsigned char)(res >> 16);
        pmac->c[4 * i + 2] = (unsigned char)(res >> 8);
        pmac->c[4 * i + 3] = (unsigned char)res;
    }
#  endif
    len += SHA_DIGEST_LENGTH;
    md = md_tail;
    SHA1_Update(&md, pmac->c, SHA_DIGEST_LENGTH);
    SHA1_Final(pmac->c, &md);

    /* verify HMAC */
    out += inp_len;
    len -= inp_len;
    {
        unsigned char *p = out + len - 1 - maxpad - SHA_DIGEST_LENGTH;
        size_t off = out - p;
        printf("off: %lx\n", off);
        unsigned int c, cmask;

        maxpad += SHA_DIGEST_LENGTH;
        for (res = 0, i = 0, j = 0; j < maxpad; j++) {
            c = p[j];
            cmask =
                ((int)(j - off - SHA_DIGEST_LENGTH)) >> (sizeof(int) *
                                                         8 - 1);
            printf("%ld: %x\n", j, ~cmask);
            res |= (c ^ pad) & ~cmask; /* ... and padding */
            cmask &= ((int)(off - 1 - j)) >> (sizeof(int) * 8 - 1);
            printf("%ld: %x\n", j, cmask);
            res |= (c ^ pmac->c[i]) & cmask;
            i += 1 & cmask;
        }
        maxpad -= SHA_DIGEST_LENGTH;

        res = 0 - ((0 - res) >> (sizeof(res) * 8 - 1));
        printf("ret (pre-res): %d\n", ret);
        ret &= (int)~res;
    }
    printf("ret: %d\n", ret);
    return ret;
}

int main() {
    int len = 32;
    unsigned char * out = malloc(len);
    int maxpad = len - SHA_DIGEST_LENGTH - 1;
    unsigned char pad = maxpad + SHA_DIGEST_LENGTH;
    memset(out, pad, len);
    aesni_cbc_hmac_sha1_cipher(out, len);
}
