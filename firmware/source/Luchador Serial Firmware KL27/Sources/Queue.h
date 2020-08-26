/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Queue
 *
 *  Created on: Dec 3, 2018
 *      Author: nick
 */

#include <stdint.h>

typedef struct
{
	uint8_t *data;
	int consumer, producer;
	int count;
	int size;
} Queue_t;


typedef enum
{
	QEX_None=0,
	QEX_Full,
	QEX_Empty,
	QEX_IndexOutOfBounds
} QException;

QException Queue_Init(Queue_t *queue, uint8_t *buf, int size);
QException Queue_Push(Queue_t *queue, uint8_t data);
QException Queue_Pop(Queue_t *queue, uint8_t *data);
QException Queue_Get(Queue_t *queue, int pos, uint8_t *data);
QException Queue_Clear(Queue_t *queue);
QException Queue_Drop(Queue_t *queue);
