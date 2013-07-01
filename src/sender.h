#ifndef SENDER_H
#define SENDER_H

/**
 * Sender thread API
 *
 * Written by Daniel Ritz
 */


/**
 * creates a new sender thread
 * @return return value of pthread_create
 */
int sender_create();

#endif
