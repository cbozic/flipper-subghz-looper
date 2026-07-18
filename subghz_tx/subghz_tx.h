#ifndef SUBGHZ_TX_H
#define SUBGHZ_TX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct SubGhzTx SubGhzTx;

    /**
     * @brief Cooperative cancellation callback, polled while a transmission is in flight.
     * @param context Caller-supplied context
     * @return true to abort the in-progress transmission early
     */
    typedef bool (*SubGhzTxShouldAbort)(void *context);

    /**
     * @brief Allocate a SubGhzTx instance and take ownership of the internal Sub-GHz radio.
     * @return SubGhzTx* pointer to a SubGhzTx instance, or NULL on failure
     */
    SubGhzTx *subghz_tx_alloc(void);

    /**
     * @brief Free a SubGhzTx instance and release the radio.
     * @param instance Pointer to a SubGhzTx instance
     */
    void subghz_tx_free(SubGhzTx *instance);

    /**
     * @brief Load and transmit a previously saved .sub file once.
     * Blocks until the transmission completes naturally or should_abort(context) returns true.
     * @param instance Pointer to a SubGhzTx instance
     * @param file_path Full path to the .sub file
     * @param should_abort Optional cooperative cancellation callback (may be NULL)
     * @param should_abort_context Context passed to should_abort
     * @return true if the file was loaded and transmitted, false if it could not be opened/parsed
     */
    bool subghz_tx_play_file(
        SubGhzTx *instance,
        const char *file_path,
        SubGhzTxShouldAbort should_abort,
        void *should_abort_context);

#ifdef __cplusplus
}
#endif

#endif
