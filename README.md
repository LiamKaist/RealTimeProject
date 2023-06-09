# RealTimeProject

## Feature number 14 :

### Objective :

Open the camera upon receiving a message from the Monitor.
  
### Explanations/Reasoning :

We created a shared variable which represents the camera and its state.
Upon receiving a message ***MESSAGE_CAM_OPEN*** on the queue from the monitor, 
the task acquires the mutex and opens the camera with the method ***Open()*** from its class.

### Added one shared variable :

#### In tasks.h :

```cpp
Camera camera; //Camera is a class from camera.h, camera.cpp
```

### Added one mutex:

#### In tasks.h :

```cpp
RT_MUTEX mutex_openCamera;
```

#### In tasks.cpp :

```cpp  
if (err = rt_mutex_create(&mutex_openCamera, NULL)) {
  cerr << "Error mutex create: " << strerror(-err) << endl << flush;
  exit(EXIT_FAILURE);
}
```

### Added one semaphore:

#### In tasks.h :

```cpp
RT_SEM sem_openCamera;
```

#### In tasks.cpp :

```cpp
if (err = rt_sem_create(&sem_openCamera, NULL, 0, S_FIFO)) {
  cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
  exit(EXIT_FAILURE);
}
```

### Added one task:

#### In tasks.h :

```cpp
/**
 * @brief Thread handling the opening of the camera lens.
 */

void OpenCamera(void *arg);
```

#### In tasks.cpp :

##### Task Creation :

```cpp
if (err = rt_task_start(&th_closeCamera, (void(*)(void*)) & Tasks::CloseCamera, this)) {
  cerr << "Error task start: " << strerror(-err) << endl << flush;
  exit(EXIT_FAILURE);
}
```

##### Task Code :

```cpp
void Tasks::OpenCamera(void *arg){
  cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
  // Synchronization barrier (waiting that all tasks are starting)
  rt_sem_p(&sem_barrier, TM_INFINITE);
  Message * msg;

  while(1){
      msg = ReadInQueue(&q_messageToMon); //No need for mutex here

      rt_sem_p(&sem_openCamera, TM_INFINITE); //Is this semaphore truly necessary?
      if(msg->GetID() == MESSAGE_CAM_OPEN){
          cout << "Opening Camera..." << endl << flush;
          rt_mutex_acquire(&mutex_openCamera, TM_INFINITE);
          camera.Open();
          rt_mutex_release(&mutex_openCamera);   
      } 
   }
}
```

## Feature number 15 :

### Objective :

Capture an Image using the camera every 100ms, then send it to the Monitor to be displayed.

### Explanations/Reasoning :

Within the OpenCamera task, we make it periodic with :

***RTIME task_period_ns= 100000000; //100 ms waiting time***

***rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(task_period_ns));***

Before grabbing an image, the mutex ***mutex_openCamera*** is acquired, the image is grabbed and attached to the pointer ***img*** .
Using a constructor from the Message class , we convert the img to a message able to be sent to the Monitor queue.
Using ***WriteInQueue()*** (the pointer to the message image is given as a parameter, a reference to the monitor queue is also provided),
the image is sent to the monitor for it to be displayed.

### Added code to openCamera task :

#### In task.cpp :
  
```cpp 
void Tasks::OpenCamera(void *arg){
  cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
  // Synchronization barrier (waiting that all tasks are starting)
  rt_sem_p(&sem_barrier, TM_INFINITE);
  Message * msg;

  RTIME task_period_ns= 100000000; //100 ms waiting time
  //Making the task periodic
  rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(task_period_ns));

  while(1){
      msg = ReadInQueue(&q_messageToMon); //No need for mutex here

      rt_sem_p(&sem_openCamera, TM_INFINITE); //Is this semaphore truly necessary?
      if(msg->GetID() == MESSAGE_CAM_OPEN){
          cout << "Opening Camera..." << endl << flush;
          rt_mutex_acquire(&mutex_openCamera, TM_INFINITE);
          camera.Open();
          rt_mutex_release(&mutex_openCamera);   
      }
    rt_mutex_acquire(&mutex_openCamera, TM_INFINITE);
    if(camera.IsOpen()){
        cout << "Camera is open" << endl << flush;
        rt_task_wait_period(NULL); //Wait for period to grab the image
        Img * img = new Img(camera.Grab());
        MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
        WriteInQueue(&q_messageToMon, msgImg); 
        rt_mutex_release(&mutex_openCamera);
    }else{
        rt_mutex_release(&mutex_openCamera);
        cout << "Camera failed to open" << endl << flush;
    }
 }
}
```

## Feature number 16 :

### Objective :

Close the camera upon receiving a message from the Monitor.

### Explanations/Reasoning :

Upon receiving a message ***MESSAGE_CAM_CLOSE*** on the queue from the monitor, 
the task acquires the mutex and closes the camera with the method ***Close()*** from its class.

### Added one mutex :

#### In tasks.h :
  
```cpp
/**
 * @brief Thread handling the closing of the camera lens.
 */

void CloseCamera(void *arg);
```

#### In tasks.cpp :

```cpp
if (err = rt_mutex_create(&mutex_closeCamera, NULL)) {
  cerr << "Error mutex create: " << strerror(-err) << endl << flush;
  exit(EXIT_FAILURE);
}
```

### Added one task :

#### In tasks.h :

#### In tasks.cpp :

##### Task creation :
  
```cpp
if (err = rt_task_create(&th_closeCamera, "th_closeCamera", 0, PRIORITY_TCAMERA, 0)) {
  cerr << "Error task create: " << strerror(-err) << endl << flush;
  exit(EXIT_FAILURE);
}
```

##### Task code :

```cpp
void Tasks::CloseCamera(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;

    while(1)
    {
        msg = ReadInQueue(&q_messageToMon);
        if(msg->GetID() == MESSAGE_CAM_CLOSE){
            cout << "Closing Camera..." << endl << flush;
            rt_mutex_acquire(&mutex_closeCamera, TM_INFINITE);
            camera.Close();
            rt_mutex_release(&mutex_closeCamera, TM_INFINITE);
        }
    }
}
```
  
