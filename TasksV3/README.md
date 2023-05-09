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

#### In tasks.cpp :

##### Task Creation :

##### Task Code :

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

#### In tasks.cpp :

##### Task creation :

##### Task code :

## Feature number 17 :

### Objective :

After receiving a Search arena request from the monitor, the supervisor stops the picture sending task
and captures a single picture on which an arena boundary will be drawn provided there is an arena in the picture. 

### Explanations/Reasoning :

### Added one task :

#### In tasks.h :

#### In tasks.cpp :

## Feature number 18 :

### Objective :

When the user asks for the position of the robot to be calculated, the task must stop the picture taking task,
take one picture and , using the saved arena, estimate the position of the robot while also annotating the current picture with that position.

### Explanations/Reasoning :

### Added one task :

#### In tasks.h :

#### In tasks.cpp :

## Feature number 19 :

### Objective :

When the user asks to stop calculating the position , the supervisor must resume the regular picture taking task.

### Explanations/Reasoning :

### Added one task :

#### In tasks.h :

#### In tasks.cpp :


  
