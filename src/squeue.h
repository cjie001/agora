#ifndef _QUEUE_H
#define _QUEUE_H

//���г�ʼ����size-����ռ�õ��ڴ��С
void* queue_init(int size);

//�ͷŶ���
void queue_release(void *h);

//�Ӷ�����ȡ����,0-�ɹ���-1-û���ݣ�-2-�ռ䲻��
int queue_get(void *h, char *data, int *data_len);

//������з�����,0-�ɹ���-1-��������
int queue_set(void *h, char *data, int data_len);

//�õ������е�Ԫ�ظ���
int queue_size(void *h);

#endif
