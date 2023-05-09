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

### Added one mutex:

#### In tasks.h :

#### In tasks.cpp :

### Added one semaphore:

#### In tasks.h :

#### In tasks.cpp :

### Added one task:

#### In tasks.h :

```cpp
RT_TASK th_openCamera;
```

#### In tasks.cpp :

```cpp

/**
 * @brief Thread handling the opening of the camera.
*/

void Tasks::OpenCamera(void *arg){
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;
    while(1){
        msg = ReadInQueue(&q_messageToMon); //No need for mutex here
        if(msg->GetID() == MESSAGE_CAM_OPEN){
            cout << "Opening Camera..." << endl << flush;
            rt_mutex_acquire(&mutex_openCamera, TM_INFINITE);
            camera.Open();
            rt_mutex_release(&mutex_openCamera);
        }
        delete(msg);
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

#### In tasks.h :

```cpp
RT_TASK th_takePictures;
```

#### In tasks.cpp :

```cpp
/**
 * @brief Thread handling the picture taking.
*/

void Tasks::TakePictures(void *arg){
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;
    RTIME task_period_ns= 100000000; //100 ms waiting time
    //Making the task periodic
    rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(task_period_ns));

    while(1){
        rt_mutex_acquire(&mutex_takePictures, TM_INFINITE);
        if ((findingArena == 0) && (sendingPosition == 0)){
            if(camera.IsOpen()){
                cout << "Camera is open" << endl << flush;
                rt_task_wait_period(NULL); //Wait for period to grab the image
                Img * img = new Img(camera.Grab());
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); 
                rt_mutex_release(&mutex_takePictures);
            }else{
                rt_mutex_release(&mutex_takePictures);
                cout << "Camera is not open" << endl << flush;
            }
        }else if(findingArena == 1){
            rt_mutex_release(&mutex_takePictures);
            rt_sem_p(&sem_arenaDone,TM_INFINITE); //Wait for the semaphore to be broadcast by the arena task
        }else if(sendingPosition == 1){
            rt_mutex_release(&mutex_takePictures);
            rt_sem_p(&sem_positionDone,TM_INFINITE); //Wait for the semaphore to be broadcast by the position task
        } 
    }
}
```

## Feature number 16 :

### Objective :

Close the camera upon receiving a message from the Monitor and stop the image sending, make sure to notify monitor.

### Explanations/Reasoning :

Upon receiving a message ***MESSAGE_CAM_CLOSE*** on the queue from the monitor, 
the task acquires the mutex and closes the camera with the method ***Close()*** from its class.

### Added one mutex :

#### In tasks.h :

#### In tasks.cpp :

### Added one task :

#### In tasks.h :

```cpp
RT_TASK th_closeCamera;
```

#### In tasks.cpp :

```cpp
/**
 * @brief Thread handling the closing of the camera lens.
 */
 
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
        delete(msg); //Has to be deleted manually
    }
}
```

## Feature number 17 :

### Objective :

After receiving a Search arena request from the monitor, the supervisor stops the picture sending task
and captures a single picture on which an arena boundary will be drawn provided there is an arena in the picture. 

### Explanations/Reasoning :

### Added one task :

### Added two shared variable :

```cpp
int findingArena = 0;
Arena * arenap;
```

#### In tasks.h :

```cpp
RT_TASK th_findArena;
```

#### In tasks.cpp :

```cpp
void Tasks::FindArena(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;
    Message * confirmation;
    Arena * arena;

    while(1)
    {
        msg = ReadInQueue(&q_messageToMon); //Waiting for a message from Monitor, have to delete it at the end!
        if(msg->GetID() == MESSAGE_CAM_ASK_ARENA)
        {
            //Stopping the taking pictures task
            rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
            findingArena=1;
            rt_mutex_release(&mutex_findArena);

            //Execute the code to find the arena
            rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
            if(camera.IsOpen()){
                Img * img = new Img(camera.Grab()); //Grab image to analyse
                arena=new img->SearchArena(); // [Not entirely sure about this]
                if (arena == NULL){
                    //Send error message to monitor
                    Message * msgSend;
                    msgSend = new Message(MESSAGE_ANSWER_NACK);
                    WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
                }else{
                    img->DrawArena(arena);
                    //Send image to monitor
                    MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                    WriteInQueue(&q_messageToMon, msgImg); //Message will be deleted by sendToMon
                }
                rt_mutex_release(&mutex_findArena);
                //Wait for confirmation from user
                confirmation = ReadInQueue(&q_messageToMon);

                if(confirmation->GetID() == MESSAGE_CAM_ARENA_CONFIRM){
                    rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
                    arenap = arena; //Not sure if a pointer can be put in shared data
                    rt_mutex_release(&mutex_findArena);
                }else if(confirmation->GetID() == MESSAGE_CAM_ARENA_INFIRM){
                    //Delete arena object
                    delete(arena); //Not sure if delete can delete any kind of pointer's content
                }
                delete(confirmation);
            }else{
                cout << "Camera is not open, cannot analyse image to find arena" << endl << flush;
                rt_mutex_release(&mutex_findArena);
            }
            rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
            findingArena=0; //Letting the pictures task start again
            rt_mutex_release(&mutex_findArena);
            rt_sem_broadcast(&sem_arenaDone); //Sending semaphore to signal periodic task OpenCamera to take pictures
        } 
        delete(msg); //Has to be deleted manually
    }
}

```

## Feature number 18 :

### Objective :

When the user asks for the position of the robot to be calculated, the task must stop the picture taking task,
take one picture and , using the saved arena, estimate the position of the robot while also annotating the current picture with that position.

### Explanations/Reasoning :

### Added one shared variable :

```cpp
int sendingPosition = 0;
```

### Added one task :

#### In tasks.h :

```cpp
RT_TASK th_positionRobot;
```

#### In tasks.cpp :

```cpp
/**
 * @brief Thread handling the position annotated picture taking.
 */

void Tasks::PositionRobot(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;
    int i;
    RTIME task_period_ns= 100000000; //100 ms waiting time
    //Making the task periodic
    rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(task_period_ns));

    while(1)
    {
        msg = ReadInQueue(&q_messageToMon);
        if(msg->GetID() == MESSAGE_CAM_POSITION){
            //Stopping the regular picture taking task
            rt_mutex_acquire(&mutex_positionRobot, TM_INFINITE);
            sendingPosition=1; 
            rt_mutex_release(&mutex_positionRobot);
            i=0;
            std::list<Position> * positionPointer;
            while(i == 0){
                
                rt_task_wait_period(NULL); //Wait for period to grab the image
                rt_mutex_acquire(&mutex_positionRobot, TM_INFINITE);
                Img * img = new Img(camera.Grab());
                //Search for the robot
                if (arenap == nullptr){
                    cout<< "No arena saved..." << endl << flush;
                }else{
                    positionPointer = new img->SearchRobot(*arenap);
                }

                if (positionPointer == nullptr){
                    cout<< "position pointer points to nothing..." << endl << flush;
                }else{
                    if(positionPointer->empty()){
                        cout << "No position detected..." << endl << flush;
                        //Send the position
                        MessagePosition * msgPos = MessagePosition(); //[Sending null position, not (-1,-1)]
                        WriteInQueue(&q_messageToMon, msgPos); //Message will be deleted by sendToMon
                    }else{
                        //Draw the position
                        img->DrawRobot(positionPointer->front()); //Using the first element of the list
                        //Send the position
                        MessagePosition * msgPos = MessagePosition(MESSAGE_CAM_POSITION, positionPointer);
                        WriteInQueue(&q_messageToMon, msgPos); //Message will be deleted by sendToMon
                    }
                }
                //Send image to monitor
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); //Message will be deleted by sendToMon
                rt_mutex_release(&mutex_positionRobot);

                rt_mutex_acquire(&mutex_positionRobot, TM_INFINITE);
                i=positionStopped;
                rt_mutex_release(&mutex_positionRobot);
                delete(pointerPosition); // Have to delete it
            }
            //Reinitialising the positionStopped variable so that this task can be launched again 
            rt_mutex_acquire(&mutex_positionRobot, TM_INFINITE);
            positionStopped=0; 
            rt_mutex_release(&mutex_positionRobot);
            //Restarting the regular picture taking task
            rt_mutex_acquire(&mutex_positionRobot, TM_INFINITE);
            sendingPosition=0; 
            rt_mutex_release(&mutex_positionRobot);
            rt_sem_broadcast(&sem_positionDone); //Sending semaphore to signal periodic task OpenCamera to take pictures
        }
        
        delete(msg); //Has to be deleted manually
    }
}

```

## Feature number 19 :

### Objective :

When the user asks to stop calculating the position , the supervisor must resume the regular picture taking task.

### Explanations/Reasoning :

### Added one shared variable :

```cpp
int positionStopped = 0;
```

### Added one task :

#### In tasks.h :

```cpp
RT_TASK th_stopPosition;
```

#### In tasks.cpp :

```cpp
/**
 * @brief Thread handling the position cancellation.
 */

void Tasks::StopPosition(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Message * msg;

    while(1)
    {
        msg = ReadInQueue(&q_messageToMon);
        if(msg->GetID() == MESSAGE_CAM_POSITION_COMPUTE_STOP){
            //Stop position
            rt_mutex_acquire(&mutex_stopPosition, TM_INFINITE);
            positionStopped=1; 
            rt_mutex_release(&mutex_stopPosition);
        }else if(msg->GetID() == MESSAGE_CAM_POSITION_COMPUTE_START){
            //Start position , in case there is an issue with the variable
            rt_mutex_acquire(&mutex_stopPosition, TM_INFINITE);
            positionStopped=0; 
            rt_mutex_release(&mutex_stopPosition);
        }
        delete(msg); //Has to be deleted manually
    }
}
```

  
