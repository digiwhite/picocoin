#ifndef __LIBCCOIN_CORE_H__
#define __LIBCCOIN_CORE_H__
/* Copyright 2012 exMULTI, Inc.
 * Distributed under the MIT/X11 software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <glib.h>
#include <ccoin/buffer.h>
#include <ccoin/buint.h>
#include <ccoin/coredefs.h>

enum service_bits {
	NODE_NETWORK	= (1 << 0),
};

static inline bool bp_valid_value(int64_t nValue)
{
	if (nValue < 0 || nValue > 21000000LL * COIN)
		return false;
	return true;
}

struct bp_address {
	uint32_t	nTime;
	uint64_t	nServices;
	unsigned char	ip[16];
	uint16_t	port;
};

static inline void bp_addr_init(struct bp_address *addr)
{
	memset(addr, 0, sizeof(*addr));
}

extern bool deser_bp_addr(unsigned int protover,
		struct bp_address *addr, struct const_buffer *buf);
extern void ser_bp_addr(GString *s, unsigned int protover, const struct bp_address *addr);
static inline void bp_addr_free(struct bp_address *addr) {}

struct bp_inv {
	uint32_t	type;
	bu256_t		hash;
};

extern void bp_inv_init(struct bp_inv *inv);
extern bool deser_bp_inv(struct bp_inv *inv, struct const_buffer *buf);
extern void ser_bp_inv(GString *s, const struct bp_inv *inv);
static inline void bp_inv_free(struct bp_inv *inv) {}

struct bp_locator {
	uint32_t	nVersion;
	GPtrArray	*vHave;		/* of bu256_t */
};

extern void bp_locator_init(struct bp_locator *locator);
extern bool deser_bp_locator(struct bp_locator *locator, struct const_buffer *buf);
extern void ser_bp_locator(GString *s, const struct bp_locator *locator);
extern void bp_locator_free(struct bp_locator *locator);

struct bp_outpt {
	bu256_t		hash;
	uint32_t	n;
};

extern void bp_outpt_init(struct bp_outpt *outpt);
extern bool deser_bp_outpt(struct bp_outpt *outpt, struct const_buffer *buf);
extern void ser_bp_outpt(GString *s, const struct bp_outpt *outpt);
static inline void bp_outpt_free(struct bp_outpt *outpt) {}

static inline bool bp_outpt_null(const struct bp_outpt *outpt)
{
	return bu256_is_zero(&outpt->hash) && outpt->n == 0xffffffff;
}

static inline bool bp_outpt_equal(const struct bp_outpt *a,
				  const struct bp_outpt *b)
{
	return (a->n == b->n) && bu256_equal(&a->hash, &b->hash);
}

struct bp_txin {
	struct bp_outpt	prevout;
	GString		*scriptSig;
	uint32_t	nSequence;
};

extern void bp_txin_init(struct bp_txin *txin);
extern bool deser_bp_txin(struct bp_txin *txin, struct const_buffer *buf);
extern void ser_bp_txin(GString *s, const struct bp_txin *txin);
extern void bp_txin_free(struct bp_txin *txin);
static inline bool bp_txin_valid(const struct bp_txin *txin) { return true; }

struct bp_txout {
	int64_t		nValue;
	GString		*scriptPubKey;
};

extern void bp_txout_init(struct bp_txout *txout);
extern bool deser_bp_txout(struct bp_txout *txout, struct const_buffer *buf);
extern void ser_bp_txout(GString *s, const struct bp_txout *txout);
extern void bp_txout_free(struct bp_txout *txout);
extern void bp_txout_set_null(struct bp_txout *txout);
extern void bp_txout_copy(struct bp_txout *dest, const struct bp_txout *src);

static inline bool bp_txout_valid(const struct bp_txout *txout)
{
	if (!txout || !txout->scriptPubKey)
		return false;
	if (!bp_valid_value(txout->nValue))
		return false;
	return true;
}

struct bp_tx {
	/* serialized */
	uint32_t	nVersion;
	GPtrArray	*vin;			/* of bp_txin */
	GPtrArray	*vout;			/* of bp_txout */
	uint32_t	nLockTime;

	/* used at runtime */
	bool		sha256_valid;
	bu256_t		sha256;
};

extern void bp_tx_init(struct bp_tx *tx);
extern bool deser_bp_tx(struct bp_tx *tx, struct const_buffer *buf);
extern void ser_bp_tx(GString *s, const struct bp_tx *tx);
extern void bp_tx_free_vout(struct bp_tx *tx);
extern void bp_tx_free(struct bp_tx *tx);
extern bool bp_tx_valid(const struct bp_tx *tx);
extern void bp_tx_calc_sha256(struct bp_tx *tx);
extern unsigned int bp_tx_ser_size(const struct bp_tx *tx);
extern void bp_tx_copy(struct bp_tx *dest, const struct bp_tx *src);

static inline bool bp_tx_coinbase(const struct bp_tx *tx)
{
	if (!tx->vin || tx->vin->len != 1)
		return false;

	struct bp_txin *txin = g_ptr_array_index(tx->vin, 0);
	if (!bp_outpt_null(&txin->prevout))
		return false;

	return true;
}

struct bp_utxo {
	bu256_t		hash;

	bool		is_coinbase;
	uint32_t	height;

	uint32_t	version;
	GPtrArray	*vout;		/* of bp_txout */
};

extern void bp_utxo_init(struct bp_utxo *coin);
extern void bp_utxo_free(struct bp_utxo *coin);
extern bool bp_utxo_from_tx(struct bp_utxo *coin, const struct bp_tx *tx,
		     bool is_coinbase, unsigned int height);

struct bp_utxo_set {
	GHashTable	*map;
};

extern void bp_utxo_set_init(struct bp_utxo_set *uset);
extern void bp_utxo_set_free(struct bp_utxo_set *uset);
extern bool bp_utxo_is_spent(struct bp_utxo_set *uset, const struct bp_outpt *outpt);
extern bool bp_utxo_spend(struct bp_utxo_set *uset, const struct bp_outpt *outpt);

static inline void bp_utxo_set_add(struct bp_utxo_set *uset,
				   struct bp_utxo *coin)
{
	g_hash_table_insert(uset->map, &coin->hash, coin);
}

static inline struct bp_utxo *bp_utxo_lookup(struct bp_utxo_set *uset,
					     const bu256_t *hash)
{
	return g_hash_table_lookup(uset->map, hash);
}


struct bp_block {
	/* serialized */
	uint32_t	nVersion;
	bu256_t		hashPrevBlock;
	bu256_t		hashMerkleRoot;
	uint32_t	nTime;
	uint32_t	nBits;
	uint32_t	nNonce;
	GPtrArray	*vtx;			/* of bp_tx */

	/* used at runtime */
	bool		sha256_valid;
	bu256_t		sha256;
};

extern void bp_block_init(struct bp_block *block);
extern bool deser_bp_block(struct bp_block *block, struct const_buffer *buf);
extern void ser_bp_block(GString *s, const struct bp_block *block);
extern void bp_block_free(struct bp_block *block);
extern void bp_block_vtx_free(struct bp_block *block);
extern void bp_block_calc_sha256(struct bp_block *block);
extern void bp_block_merkle(bu256_t *vo, const struct bp_block *block);
extern GArray *bp_block_merkle_tree(const struct bp_block *block);
extern GArray *bp_block_merkle_branch(const struct bp_block *block,
			       const GArray *mrktree,
			       unsigned int txidx);
extern void bp_check_merkle_branch(bu256_t *hash, const bu256_t *txhash_in,
			    const GArray *mrkbranch, unsigned int txidx);
extern bool bp_block_valid(struct bp_block *block);
extern unsigned int bp_block_ser_size(const struct bp_block *block);

static inline void bp_block_copy_hdr(struct bp_block *dest,
				     const struct bp_block *src)
{
	memcpy(dest, src, sizeof(*src));
	dest->vtx = NULL;
}

static inline int64_t bp_block_value(unsigned int height, int64_t fees)
{
	int64_t subsidy = 50LL * COIN;
	subsidy >>= (height / 210000);
	return subsidy + fees;
}

#endif /* __LIBCCOIN_CORE_H__ */
