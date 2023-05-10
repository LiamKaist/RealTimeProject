/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21


/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_mutex_create(&mutex_camera, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_arenap, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_findingArena, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_arenaConfirmed, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_arenaInStock, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_sendingPosition, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_mutex_create(&mutex_positionStopped, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    
    if (err = rt_sem_create(&sem_camOpen, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_sem_create(&sem_camClose, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
   
            
    if (err = rt_sem_create(&sem_startArena, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_sem_create(&sem_confirmation, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
            
    if (err = rt_sem_create(&sem_arenaDone, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_sem_create(&sem_positionDone, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_create(&th_openCamera, "th_openCamera", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_takePictures, "th_takePictures", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_closeCamera, "th_closeCamera", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_findArena, "th_findArena", 0, PRIORITY_TCAMERA, 0)) { //Peut etre faudrait-il changer une priorité?
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_create(&th_positionRobot, "th_positionRobot", 0, PRIORITY_TCAMERA, 0)) { 
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_openCamera, (void(*)(void*)) & Tasks::OpenCamera, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_start(&th_takePictures, (void(*)(void*)) & Tasks::TakePictures, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    

    if (err = rt_task_start(&th_closeCamera, (void(*)(void*)) & Tasks::CloseCamera, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_findArena, (void(*)(void*)) & Tasks::FindArena, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    if (err = rt_task_start(&th_positionRobot, (void(*)(void*)) & Tasks::PositionRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    
    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            delete(msgRcv);
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)){
            rt_sem_broadcast(&sem_camOpen);
        } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)){
            rt_sem_broadcast(&sem_camClose);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA)){
            rt_sem_broadcast(&sem_startArena);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_CONFIRM)){
            rt_mutex_acquire(&mutex_arenaConfirmed,TM_INFINITE);
            arenaConfirmed=1;
            rt_mutex_release(&mutex_arenaConfirmed);
            rt_sem_broadcast(&sem_confirmation);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_INFIRM)){
            rt_mutex_acquire(&mutex_arenaConfirmed,TM_INFINITE);
            arenaConfirmed=0;
            rt_mutex_release(&mutex_arenaConfirmed);
            rt_sem_broadcast(&sem_confirmation);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION)){
            //Stopping the regular picture taking task
            rt_mutex_acquire(&mutex_sendingPosition,TM_INFINITE);
            sendingPosition=1;
            rt_mutex_release(&mutex_sendingPosition);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_STOP)){
            rt_mutex_acquire(&mutex_positionStopped,TM_INFINITE);
            positionStopped=1;
            rt_mutex_release(&mutex_positionStopped);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_START)){
            rt_mutex_acquire(&mutex_positionStopped,TM_INFINITE);
            positionStopped=0;
            rt_mutex_release(&mutex_positionStopped);
        }
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {

        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        cout << "Start robot without watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(robot.StartWithoutWD());
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            robot.Write(new Message((MessageID)cpMove));
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
    }
}

/**
 * @brief Thread handling the opening of the camera.
*/

void Tasks::OpenCamera(void *arg){
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);  
    
    while(1){
        rt_sem_p(&sem_camOpen, TM_INFINITE); 
        cout << "Opening Camera..." << endl << flush;
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        if(!camera.IsOpen()){
            camera.Open();
        }
        //Checking whether the camera was opened or not and notifying the monitor
        Message * msgSend;
        if (!camera.IsOpen()){
            msgSend = new Message(MESSAGE_ANSWER_NACK);
            WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
            WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
        }
        rt_mutex_release(&mutex_camera);
    }
}

/**
 * @brief Thread handling the picture taking.
*/

void Tasks::TakePictures(void *arg){
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while(1){
        rt_task_wait_period(NULL); //Wait for period to grab the image
        rt_mutex_acquire(&mutex_findingArena, TM_INFINITE);
        rt_mutex_acquire(&mutex_sendingPosition, TM_INFINITE);
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        rt_mutex_acquire(&mutex_arenaInStock,TM_INFINITE);
        rt_mutex_acquire(&mutex_arenap, TM_INFINITE);

        if ((findingArena == 0) && (sendingPosition == 0) && (arenaInStock == 0)){
            if(camera.IsOpen()){
                cout << "Camera is open" << endl << flush;
                Img * img = new Img(camera.Grab());
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); 
            }else{
                cout << "Camera is not open" << endl << flush;
            }
        }else if(findingArena == 1){
            rt_sem_p(&sem_arenaDone,TM_INFINITE); //Wait for the semaphore to be broadcast by the arena task
        }else if(sendingPosition == 1){
            rt_sem_p(&sem_positionDone,TM_INFINITE); //Wait for the semaphore to be broadcast by the position task
        } else if ((findingArena == 0) && (sendingPosition == 0) && (arenaInStock == 1)){ //Drawing the stored arena on every picture
           if(camera.IsOpen()){
                cout << "Camera is open and arena is in stock" << endl << flush;
                Img * img = new Img(camera.Grab());
                img->DrawArena(*arenap);
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); 
            }else{
                cout << "Camera is not open" << endl << flush;
            } 
        }
        rt_mutex_release(&mutex_arenap);
        rt_mutex_release(&mutex_arenaInStock);
        rt_mutex_release(&mutex_camera);
        rt_mutex_release(&mutex_sendingPosition);
        rt_mutex_release(&mutex_findingArena);
    }
}

/**
 * @brief Thread handling the closing of the camera lens.
 */

void Tasks::CloseCamera(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    while(1){
        rt_sem_p(&sem_camClose, TM_INFINITE);
        cout << "Closing Camera..." << endl << flush;
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        if(camera.IsOpen()){
            camera.Close();
        }

        //Checking whether or not the camera was closed and notifying the monitor
        Message * msgSend;
        if (camera.IsOpen()){
            msgSend = new Message(MESSAGE_ANSWER_NACK);
            WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
            WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
        }
        rt_mutex_release(&mutex_camera);
    }   
}

void Tasks::FindArena(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    Arena * arena;

    while(1)
    {
        rt_sem_p(&sem_startArena, TM_INFINITE);
        //Stopping the taking pictures task
        rt_mutex_acquire(&mutex_findingArena, TM_INFINITE);
        findingArena=1;
        rt_mutex_release(&mutex_findingArena);

        //Execute the code to find the arena
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        if(camera.IsOpen()){
            Img * img = new Img(camera.Grab()); //Grab image to analyse
            arena=new Arena(img->SearchArena()); // [Not entirely sure about this] Put parentheses
            if (arena == nullptr){
                //Send error message to monitor
                Message * msgSend;
                msgSend = new Message(MESSAGE_ANSWER_NACK);
                WriteInQueue(&q_messageToMon, msgSend); //Message will be deleted by sendToMon
            }else{
                img->DrawArena(*arena);
                //Send image to monitor
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); //Message will be deleted by sendToMon
            }

            //Wait for confirmation from user
            cout << "Please confirm the arena (or not)..." << endl << flush;

            rt_sem_p(&sem_confirmation, TM_INFINITE);
            rt_mutex_acquire(&arenaConfirmed,TM_INFINITE);
            if(arenaConfirmed == 1){
                rt_mutex_acquire(&mutex_arenap, TM_INFINITE);
                arenap = arena; //Not sure if a pointer can be put in shared data
                rt_mutex_release(&mutex_arenap);
            }else if(arenaConfirmed == 0){
                //Delete arena object
                delete(arena); //Not sure if delete can delete any kind of pointer's content
            }
            rt_mutex_release(&arenaConfirmed);
        }else{
            cout << "Camera is not open, cannot analyse image to find arena" << endl << flush;
        }

        rt_mutex_release(&mutex_camera);
        rt_mutex_acquire(&mutex_findingArena, TM_INFINITE);
        findingArena=0; //Letting the pictures task start again
        rt_mutex_release(&mutex_findingArena);
        rt_sem_broadcast(&sem_arenaDone); //Sending semaphore to signal periodic task OpenCamera to take pictures
    }
}

/**
 * @brief Thread handling the position annotated picture taking.
 */

void Tasks::PositionRobot(void *arg)
{
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    //Making the task periodic
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while(1)
    {
        rt_task_wait_period(NULL);
        rt_mutex_acquire(&mutex_sendingPosition, TM_INFINITE);
        if(sendingPosition == 1){
            std::list<Position> * positionPointer;
            std::list<Position> positionInstance;

            rt_mutex_acquire(&mutex_positionStopped,TM_INFINITE);
            while(positionStopped == 0){
                
                //Wait for period to grab the image
                rt_mutex_acquire(&mutex_camera, TM_INFINITE);
                rt_mutex_acquire(&mutex_arenap,TM_INFINITE);
                Img * img = new Img(camera.Grab());
                //Search for the robot
                if (arenap == nullptr){
                    cout<< "No arena saved..." << endl << flush;
                }else{
                    positionInstance=img->SearchRobot(*arenap);
                    positionPointer = &positionInstance;
                }

                if (positionPointer == nullptr){
                    cout<< "position pointer points to nothing..." << endl << flush;
                }else{
                    if(positionPointer->empty()){
                        cout << "No position detected..." << endl << flush;
                        //Send the position
                        MessagePosition * msgPos = new MessagePosition(); //[Sending null position, not (-1,-1)]
                        WriteInQueue(&q_messageToMon, msgPos); //Message will be deleted by sendToMon
                    }else{
                        //Draw the position
                        img->DrawRobot(positionPointer->front()); //Using the first element of the list
                        //Send the position
                        MessagePosition * msgPos = new MessagePosition(MESSAGE_CAM_POSITION, positionPointer->front());
                        WriteInQueue(&q_messageToMon, msgPos); //Message will be deleted by sendToMon
                    }
                }
                //Send image to monitor
                MessageImg * msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgImg); //Message will be deleted by sendToMon
                rt_mutex_release(&mutex_arenap);
                rt_mutex_release(&mutex_camera);
                delete(positionPointer); // Have to delete it
            }

            rt_mutex_release(&mutex_positionStopped);

            //Reinitialising the positionStopped variable so that this task can be launched again 
            rt_mutex_acquire(&mutex_positionStopped, TM_INFINITE);
            positionStopped=0; 
            rt_mutex_release(&mutex_positionStopped);
            //Restarting the regular picture taking task
            //The mutex for sendingPosition is already acquired
            sendingPosition=0; 
            rt_sem_broadcast(&sem_positionDone); //Sending semaphore to signal periodic task OpenCamera to take pictures
        }
    }
    rt_mutex_release(&mutex_sendingPosition);
}


/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

