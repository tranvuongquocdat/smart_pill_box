/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */


import React,{useState,useEffect,useCallback} from 'react';
import PushNotification from "react-native-push-notification";
import AsyncStorage from '@react-native-community/async-storage'
import {
  StyleSheet,Keyboard,PermissionsAndroid,Platform,LogBox,AppState, NativeEventEmitter, // for emitting events for the BLE manager
  NativeModules
} from 'react-native';
import{
  Box,
  NativeBaseProvider,
  HStack,
  Text,
  Button,
  Center,
  VStack,
  Spacer,
  Switch,
  Pressable,
  Modal,
  FormControl,
  Input,Toast,Spinner,ScrollView
} from 'native-base'
import BleManager from 'react-native-ble-manager'; // for talking to BLE peripherals
const BleManagerModule = NativeModules.BleManager;
const bleManagerEmitter = new NativeEventEmitter(BleManagerModule);
const CMD_CODE ={
  disale_cell:0x05,
  set_time:0x04,
  clear_history:0x03,
  read_history:0x02,
  set_cell:0x01
}
// import NetInfo from "@react-native-community/netinfo";

// const unsubscribe = NetInfo.addEventListener(state => {
// console.log("Connection type", state.type);
// console.log("Is connected?", state.isConnected);
// });
 // Ignore log notification by message:
 LogBox.ignoreLogs(['Warning: ...']);
 
 // Ignore all log notifications:
 LogBox.ignoreAllLogs();
 PushNotification.configure({
  // (optional) Called when Token is generated (iOS and Android)
  onRegister: function (token) {
    console.log("TOKEN:", token);
  },

  // (required) Called when a remote is received or opened, or local notification is opened
  onNotification: function (notification) {
    console.log("NOTIFICATION:", notification);
    // (required) Called when a remote is received or opened, or local notification is opened
    // notification.finish(PushNotificationIOS.FetchResult.NoData);
    // notification.finish(PushNotification.FetchResult.NoData);
  },


  // IOS ONLY (optional): default: all - Permissions to register.
  permissions: {
    alert: true,
    badge: true,
    sound: true,
  },
  popInitialNotification: true,
  requestPermissions: Platform.OS === 'ios',
});
PushNotification.createChannel(
  {
    channelId: "notifi", // (required)
    channelName: "My channel", // (required)
 // (optional) default: true// (optional) See `soundName` parameter of `localNotification` function
    importance: 4, // (optional) default: 4. Int value of the Android notification importance
    vibrate: true, // (optional) default: true. Creates the default vibration patten if true.
    invokeApp: true,
    
  },
  (created) => console.log(`createChannel returned '${created}'`) // (optional) callback returns whether the channel was created, false means it already existed.
);
const Toolbar=()=>{
  return(
    <Box
        bg="blue.500"
        width="100%"
        py="4"
        px="2"
        shadow="5"
      >
       <HStack justifyContent="space-between" >
          <Text style={styles.normalText}>Trang ch???</Text>
          
        </HStack>
    </Box>
  )
}

// const Item =({dt,index})=>{
//   return(
    
//   )
// }

const Status=({status})=>{
  return(
    <Box
      bg={status?"green.400":"red.400"}
      width="95%"
      p="4"
      rounded="lg"
      shadow="2"
      marginTop="2"
      marginBottom="2"
    >
      <HStack justifyContent="space-between" >
        <Text style={styles.normalText}>Tr???ng th??i</Text>
        <Text  style={styles.normalText}>{status?"Online":"Offline"}</Text>
      </HStack>
    </Box>
  )
}

const App = () => {

  const dt=[
    {
      name:"Ng??n 1",
      item_name:"Thu???c A",
      time:"8:00",
      status:false,
      lieu_luong:0,
      over:0
      
    },
    {
      name:"Ng??n 2",
      item_name:"Thu???c A",
      time:"8:30",
      status:false,
      lieu_luong:0,
      over:0
    },
    {
      name:"Ng??n 3",
      item_name:"Thu???c A",
      time:"12:30",
      status:false,
      lieu_luong:0,
      over:0
    },
    {
      name:"Ng??n 4",
      item_name:"Thu???c A",
      time:"8:30",
      status:false,
      lieu_luong:0,
      over:0
    }
  ]

  const service_UUID ="6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
  const service_RX ="6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
  const service_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
  const DEVICE_NAME = 'Smart medicine chest'
  const [data,setData] = useState(dt)
  const [enable,setEnable] = useState(false)

  const [bleStatus,setBleStatus] = useState(false)
  const [name,setName] = useState("")
  const [lieu,setLieu] = useState("")
  const [itemName,setItemName] = useState("")
  const [time1,setTime1]= useState("")
  const [time2,setTime2]= useState("")
  const [time3,setTime3]= useState("")
  // const [listDevice,setListDevice]=useState([])
  const [device,setDevice]= useState(null)
  const [loading,setLoading] = useState(false)

  let found =0
  // const setItemColor = (i)=>{
  //   if(data[i].status == true){
  //     if(data[i].over>3)
  //     {
  //       return "red.400"
  //     }
  //     else
  //     {
  //       return "ble.400"
  //     }
  //   }
  //   else
  //   {

  //   }
  // }
  const onChangeStatus=(index)=>{
    // data[index].status = !data[index].status
    // console.log("state change:",index)
      if(device!==null)
      {
        //send lenh setting
        if(data[index].status == false)
        {

          let cmd=[CMD_CODE.set_cell,index+1,data[index].lieu_luong]
          let time_arr = data[index].time.split(", ")
          let len = time_arr.length
          if(len>=1)
          {
            let arr = time_arr[0].split(":")
            cmd.push(parseInt(arr[0]))
            cmd.push(parseInt(arr[1]))
          }
          else
          {
            cmd.push(0xaa)
            cmd.push(0xaa)
          }

          if(len>=2)
          {
            let arr = time_arr[1].split(":")
            cmd.push(parseInt(arr[0]))
            cmd.push(parseInt(arr[1]))
          }
          else
          {
            cmd.push(0xaa)
            cmd.push(0xaa)
          }

          if(len>=3)
          {
            let arr = time_arr[2].split(":")
            cmd.push(parseInt(arr[0]))
            cmd.push(parseInt(arr[1]))
          }
          else
          {
            cmd.push(0xaa)
            cmd.push(0xaa)
          }

          //send set data
          BleManager.write(device,service_UUID,service_RX,cmd)
          .then(res=>{
            console.log("gui lenh thanh cong setting ngan ",index+1)
            let newData = [...data]
            newData[index].status= !newData[index].status
            setData([...newData])
            AsyncStorage.setItem("data",JSON.stringify(newData))
            Toast.show({
              title:"G???i thi???t l???p ng??n thu???c th??nh c??ng!"
            })
          })
          .catch(err=>{
            console.log("set cell error:",err)
            Toast.show({
              title:"G???i thi???t l???p ng??n thu???c th???t b???i!"
            })
          })
        }
        //send lenh disable
        else
        {
          let cmd=[CMD_CODE.disale_cell,index+1]
          //send set data
          BleManager.write(device,service_UUID,service_RX,cmd)
          .then(res=>{
            console.log("gui lenh thanh cong disable ngan ",index+1)
            let newData = [...data]
            newData[index].status= !newData[index].status
            setData([...newData])
            AsyncStorage.setItem("data",JSON.stringify(newData))
            Toast.show({
              title:"G???i thi???t l???p ng??n thu???c th??nh c??ng!"
            })
          })
          .catch(err=>{
            console.log("set cell error:",err)
            Toast.show({
              title:"G???i thi???t l???p ng??n thu???c th???t b???i!"
            })
          })
        }
        
        
      }
      else
      {
        Toast.show({
          title:"Vui l??ng k???t n???i thi???t b??? ????? thi???t l???p ng??n thu???c!"
        })
      }
      
    // console.log("new data:",newData)
  }
  const onEdit=(index)=>{
    let timeArr = data[index].time.split(", ")
    console.log(timeArr)
    let len = timeArr.length
    setName(data[index].name)
    setLieu(data[index].lieu_luong.toString())
    setItemName(data[index].item_name)
    setTime1(len>=1?timeArr[0]:"")
    setTime2(len>=2?timeArr[1]:"")
    setTime3(len>=3?timeArr[2]:"")
    
    setEnable(true)
  }
  const onSave = ()=>{
    Keyboard.dismiss();
    if(device===null)
    {
      Toast.show({
        title:"Vui l??ng k???t n???i thi???t b??? ????? thi???t l???p ng??n thu???c!"
      })
      return
    }
    let num = parseInt(lieu)
    if(num>9)
    {
      Toast.show({
        title:"Vui l??ng nh???p li???u l?????ng t???i ??a 9 vi??n/l???n!"
      })
      return
    }
    let isValid = /^([0-1]?[0-9]|2[0-4]):([0-5][0-9])(:[0-5][0-9])?$/
    let arrTime =[]
    let err=[]
    let cmd = [CMD_CODE.set_cell,0,parseInt(lieu)]
    if(time1.length>0)
    {
      let valid = isValid.test(time1)
      if(!valid){
        err.push("1")
      }
      else{
        arrTime.push(time1)
        let arr = time1.split(":")
        cmd.push(parseInt(arr[0]))
        cmd.push(parseInt(arr[1]))
      }
    }
    else
    {
      cmd.push(0xaa)
      cmd.push(0xaa)
    }
    if(time2.length>0)
    {
      let valid = isValid.test(time2)
      if(!valid){
        err.push("2")
      }
      else{
        arrTime.push(time2)
        let arr = time2.split(":")
        cmd.push(parseInt(arr[0]))
        cmd.push(parseInt(arr[1]))
      }
    }
    else
    {
      cmd.push(0xaa)
      cmd.push(0xaa)
    }


    if(time3.length>0)
    {
      let valid = isValid.test(time3)
      if(!valid){
        err.push("3")
      }
      else{
        arrTime.push(time3)
        let arr = time3.split(":")
        cmd.push(parseInt(arr[0]))
        cmd.push(parseInt(arr[1]))
      }
    }
    else
    {
      cmd.push(0xaa)
      cmd.push(0xaa)
    }

    if(err.length>0)
    {
      // console.log("error data time",err)
      Toast.show({
        title:"Sai ?????nh d???ng th???i gian "+err.join(", ") + ". Y??u c???u nh???p theo ?????nh d???ng HH:mm!"
      })
    }


    else
    {
      
      
      let newData = data
      let index = newData.findIndex(el=>el.name == name)

      //gui lenh setting
      if(data[index].status == true)
      {
        cmd[1]=index+1
        BleManager.write(device,service_UUID,service_RX,cmd)
        .then(res=>{
          console.log("save and send setting sucesss")
          
          newData[index]={...newData[index],item_name:itemName,time:arrTime.join(", "),lieu_luong:parseInt(lieu)}
          setData([...newData])
          setEnable(false)
          AsyncStorage.setItem("data",JSON.stringify(newData))
          Toast.show({
            title:"L??u thi???t l???p ng??n thu???c th??nh c??ng!"
          })
        })
        .catch(err=>{
          console.log("gui va save failed:",err)
          Toast.show({
            title:"L??u thi???t l???p ng??n thu???c th???t b???i!"
          })
        })
      }

      else
      {

        newData[index]={...newData[index],item_name:itemName,time:arrTime.join(", "),lieu_luong:parseInt(lieu)}
        setData([...newData])
        setEnable(false)
        AsyncStorage.setItem("data",JSON.stringify(newData))
        Toast.show({
          title:"L??u thi???t l???p ng??n thu???c th??nh c??ng!"
        })
      }
      
      
    }
  
    
  }

  const onScan =async()=>{
    try {
      await BleManager.start({ showAlert: false });
    } catch (error) {
      console.log('Cannot initialize BLE Module');
      Toast.show({
        title:"Vui l??ng b???t bluetooth!"
      })
      return;
    }
    found=0
    setLoading(true)
    await BleManager.scan([],4,false)
    
  }


  const onHandleCon = async()=>{
    
    //no connection
    if(!bleStatus){
      
      console.log("scan device")
      
      await onScan()
      // setTimeout(()=>{
      //   setLoading(false)
      // },5000)
    }
    else if (bleStatus == true && device!==null){
      try
      {
        console.log("disconnect device "+device)
        await BleManager.disconnect(device,false)
        setBleStatus(false)
        setDevice(null)
        console.log("disconnect success")
      }
      catch(err){
        console.log(err)
      }
    }
  }

  const onDeleteHistory = ()=>{
    // try{
      // const per = await BleManager.retrieveServices(device,[service_UUID]);
      // if(!per)
      // {
      //   console.log("get history error")
      //   return
      // }
      BleManager.write(device,service_UUID,service_RX,[CMD_CODE.clear_history])
      .then(res=>{
        let newData = data
        for(let i=0;i<newData.length;i++){
          newData[i].over =0
        }
        // console.log(newData)
        setData([...newData])
        AsyncStorage.setItem('data',JSON.stringify(newData))
        Toast.show({
          title:"X??a l???ch s???  u???ng thu???c th??nh c??ng!"
        })
      })
      .catch(err=>{
        console.log("get history error:",err)
      })
     
  }




  const onDiscoverDevice =(d)=>{
    console.log("new device:"+d.name)
    if(d.name == DEVICE_NAME)
    {
      console.log("found device")
      found=1
      BleManager.stopScan()
      .then(res=>{
        return BleManager.connect(d.id)
       
      })
      .then(()=>{
        console.log("connect success!")
        setDevice(d.id)
        setBleStatus(true)
        BleManager.retrieveServices(d.id,[service_UUID])
        .then((per)=>{
          if(per)
          {
            return BleManager.startNotification(d.id,service_UUID,service_TX)
          }
          else
          {
            console.log("error notifi")
          }
          
        })
        .then(res=>{
          console.log("notifi oke")
          let now = new Date()
          let hour = now.getHours()
          let min = now.getMinutes()
          return BleManager.write(d.id,service_UUID,service_RX,[CMD_CODE.set_time,hour,min])
        })
        .then(res=>{
          console.log("send current time data")
        })
        .catch(err=>{
          console.log("notifi error:",err)
        })
        
      })
      .catch(err=>{
        console.log(err)
        Toast.show({
          title:"L???i k???t n???i thi???t b???!"
        })
      })
    }
  }

  const onDisconnect =(e)=>{
    console.log("mat ket noi",e.status)
    if(e.status >0){
      console.log("mat ket noi thiet bi")
      setDevice(null)
      setBleStatus(false)
      found =0
      if(Toast.isActive) 
      {
        Toast.show({
          title:"M???t k???t n???i thi???t b???!"
        })
      }
      
      
      
    }
  }
  const onStopScan = ()=>{
    setLoading(false)
    console.log("stop scanning:",found)
    if(found ==0){
      Toast.show({
        title:"Kh??ng t??m th???y thi???t b???!"
      })
    }
   
    
  }
  const onUpdateData = async(dt)=>{
    if(!dt.value){
      return
    }
    console.log('data update:',dt.value)
    let arr = dt.value
    if(arr[0] === CMD_CODE.read_history)
    {
      // console.log("update history ",data)
      
      let oldDt = await AsyncStorage.getItem('data')
      let newData = JSON.parse(oldDt)
      for(let i=0;i<newData.length;i++)
      {
        newData[i].over = arr[i+1]
      }
      //console.log(newData)
      setData([...newData])
      AsyncStorage.setItem('data',JSON.stringify(newData))
      let index = []
      for(let i=0;i<newData.length;i++)
      {
        if(newData[i].status === true && newData[i].over>3)
        {
          index.push(i+1)
        }
      }
      if(index.length>0)
      {
        let message = "Ng??n " + index.join(", ")+ " b??? u???ng thu???c qu?? 3 l???n!"
        PushNotification.localNotification({
          channelId:"notifi",
          title:"Th??ng b??o",
          message:message,
          vibration: 1000,
          smallIcon:"ic_launcher",
          largeIcon:"ic_launcher",
          bigLargeIcon:"ic_launcher",
          playSound: true,
          soundName:"sound.wav"
          
        })
      }
      
    }
  }
  useEffect(()=>{
    (async() =>{
      let m = await AsyncStorage.getItem("data")
      // console.log(dt)
      if(m!==null && m.length>0)
      {
        let parse = JSON.parse(m)
        setData(parse)
        
      }  
      else
      {
        await AsyncStorage.setItem("data",JSON.stringify(dt))
      }
    })()
    const handleDiscover = bleManagerEmitter.addListener(
      'BleManagerDiscoverPeripheral',
      onDiscoverDevice
    )

    const handleDisconnect = bleManagerEmitter.addListener(
      'BleManagerDisconnectPeripheral',
      onDisconnect
    )

    const handleStopScan = bleManagerEmitter.addListener(
      'BleManagerStopScan',
      onStopScan
    )
    const handleUpdate = bleManagerEmitter.addListener(
      'BleManagerDidUpdateValueForCharacteristic',
      onUpdateData
    )
     
    if (Platform.OS === 'android' && Platform.Version >= 23) {
      PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION).then((result) => {
          if (result) {
              console.log("Permission is OK");
              (async function(){onScan()})() 
              // this.retrieveConnected()
          } else {
              PermissionsAndroid.request(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION).then((result) => {
                  if (result) {
                  } else {
                      console.log("User refuse");
                      Toast.show({
                        title:"Vui l??ng cho ph??p v??? tr?? ????? s??? d???ng Bluetooth"
                      })
                  }
              });
          }
      });
    }
   
    // onScan()
   
   
    return()=>{
      console.log("exit app")
      handleDiscover.remove()
      handleDisconnect.remove()
      handleStopScan.remove()
      handleUpdate.remove()
      
      // manager.stopDeviceScan()
      // manager.destroy()
      
      
    }
  },[])


  // const renderItem =({item,index})
  return (
    <NativeBaseProvider >
      
      <Toolbar />
      <ScrollView>
      <Center >
        
        <Status status={bleStatus}/>
       
        <Pressable onPress={()=>onEdit(0)}  w="95%">
          <VStack 
            bg={data[0].status?(data[0].over>3?"red.400":"blue.200"):"coolGray.100"}
            p="2"
            marginBottom="2"
            rounded="md"
            w="100%"
            shadow="2"
          >
            <HStack p="1.5"   justifyContent="space-between" alignItems="center" borderBottomWidth="1" borderBottomColor="coolGray.200" >
              <Text style={styles.sectionTitle}>{data[0].name}, b??? u???ng {data[0].over} l???n</Text>
              <Switch colorScheme="blue"  isChecked={data[0].status} onToggle={()=>onChangeStatus(0)} />
            </HStack>
            <Spacer/>
            <Text fontSize="16" px="2" fontWeight="500">T??n thu???c: {data[0].item_name}</Text>
            <Text fontSize="16" px="2" fontWeight="500">Li???u l?????ng: {data[0].lieu_luong} vi??n/l???n</Text>
            <Text fontSize="16" px="2" fontWeight="500">Th???i gian u???ng: {data[0].time}</Text>
          </VStack>
        </Pressable>

        <Pressable onPress={()=>onEdit(1)}  w="95%">
          <VStack 
            bg={data[1].status?(data[1].over>3?"red.400":"blue.200"):"coolGray.100"}
            p="2"
            marginBottom="2"
            rounded="md"
            w="100%"
            shadow="2"
          >
            <HStack p="1.5"   justifyContent="space-between" alignItems="center" borderBottomWidth="1" borderBottomColor="coolGray.200" >
              <Text  style={styles.sectionTitle}>{data[1].name}, b??? u???ng {data[1].over} l???n</Text>
              <Switch colorScheme="blue"  isChecked={data[1].status} onToggle={()=>onChangeStatus(1)} />
            </HStack>
            <Spacer/>
            <Text fontSize="16" px="2" fontWeight="500">T??n thu???c: {data[1].item_name}</Text>
            <Text fontSize="16" px="2" fontWeight="500">Li???u l?????ng: {data[1].lieu_luong} vi??n/l???n</Text>
            <Text fontSize="16" px="2" fontWeight="500">Th???i gian u???ng: {data[1].time}</Text>
          </VStack>
        </Pressable>

        <Pressable onPress={()=>onEdit(2)}  w="95%">
          <VStack 
            bg={data[2].status?(data[2].over>3?"red.400":"blue.200"):"coolGray.100"}
            p="2"
            marginBottom="2"
            rounded="md"
            w="100%"
            shadow="2"
          >
            <HStack p="1.5"   justifyContent="space-between" alignItems="center" borderBottomWidth="1" borderBottomColor="coolGray.200" >
              <Text style={styles.sectionTitle}>{data[2].name}, b??? u???ng {data[2].over} l???n</Text>
              <Switch colorScheme="blue" isChecked={data[2].status} onToggle={()=>onChangeStatus(2)} />
            </HStack>
            <Spacer/>
            <Text fontSize="16" px="2" fontWeight="500">T??n thu???c: {data[2].item_name}</Text>
            <Text fontSize="16" px="2" fontWeight="500">Li???u l?????ng: {data[2].lieu_luong} vi??n/l???n</Text>
            <Text fontSize="16" px="2" fontWeight="500">Th???i gian u???ng: {data[2].time}</Text>
          </VStack>
        </Pressable>

        <Pressable onPress={()=>onEdit(3)}  w="95%">
          <VStack 
            bg={data[3].status?(data[3].over>3?"red.400":"blue.200"):"coolGray.100"}
            p="2"
            marginBottom="2"
            rounded="md"
            w="100%"
            shadow="2"
          >
            <HStack p="1.5"   justifyContent="space-between" alignItems="center" borderBottomWidth="1" borderBottomColor="coolGray.200" >
              <Text style={styles.sectionTitle}>{data[3].name}, b??? u???ng {data[3].over} l???n</Text>
              <Switch colorScheme="blue" isChecked={data[3].status} onToggle={()=>onChangeStatus(3)} />
            </HStack>
            <Spacer/>
            <Text fontSize="16" px="2" fontWeight="500">T??n thu???c: {data[3].item_name}</Text>
            <Text fontSize="16" px="2" fontWeight="500">Li???u l?????ng: {data[3].lieu_luong} vi??n/l???n</Text>
            <Text fontSize="16" px="2" fontWeight="500">Th???i gian u???ng: {data[3].time}</Text>
          </VStack>
        </Pressable>

        <Modal isOpen={enable} onClose={() => setEnable(false)}>
          <Modal.Content maxWidth="400px" >
            <Modal.CloseButton />
            <Modal.Header >{name}</Modal.Header>
            <Modal.Body>
              <FormControl>
                <FormControl.Label fontSize="18">T??n thu???c</FormControl.Label>
                <Input colorScheme="blue" value={itemName} onChangeText={text=>setItemName(text)}   placeholder="Thu???c h??? s???t" />
              </FormControl>
              <FormControl mt="2">
                <FormControl.Label fontSize="18">Li???u l?????ng(vi??n/l???n)</FormControl.Label>
                  <Input colorScheme="blue" keyboardType='numeric' value={lieu} onChangeText={text=>setLieu(text.replace(/[^0-9]/g, ''))}   placeholder="3 vi??n/l???n" />
              </FormControl>
              <FormControl mt="2"  >
                <FormControl.Label fontSize="18">Th???i gian 1</FormControl.Label>
                <Input colorScheme="blue" value={time1} onChangeText={text=>setTime1(text)}  placeholder="8:30" />
              </FormControl>
              <FormControl mt="2">
                <FormControl.Label fontSize="18">Th???i gian 2</FormControl.Label>
                <Input colorScheme="blue" value={time2} onChangeText={text=>setTime2(text)} placeholder="12:30" />
              </FormControl>
              <FormControl mt="2">
                <FormControl.Label fontSize="18">Th???i gian 3</FormControl.Label>
                <Input colorScheme="blue" value={time3} onChangeText={text=>setTime3(text)} placeholder="18:30" />
              </FormControl>
            </Modal.Body>
            <Modal.Footer>
              <Button.Group space={2}>
                <Button
                  variant="ghost"
                  px="10"
                  colorScheme="blueGray"
                  fontSize="18"
                  onPress={() => {
                    setEnable(false)
                  }}
                >
                  H???y
                </Button>
                <Button
                  bg="blue.400"
                  px="10"
                  fontSize="18"
                  onPress={() => onSave()}
                >
                  L??u
                </Button>
              </Button.Group>
            </Modal.Footer>
          </Modal.Content>
        </Modal>
        <Modal isOpen={loading} onClose={() => setLoading(false)}>
          <Modal.Content maxWidth="200px" >
            <Modal.Body>
              <Spinner  color="blue.400"></Spinner>
            </Modal.Body>
          </Modal.Content>
        </Modal>
        
      <HStack mt="2" w="95%" justifyContent='space-between'>
        <Button
          bg="blue.400"
          p="3"
          w="45%"
          style={styles.normalText}
          onPress ={()=>onHandleCon()}
        >
          {!bleStatus?"K???t n???i":"H???y k???t n???i"}
        </Button>
        {/* <Button
          bg="blue.400"
          p="3"
          w="30%"
          isDisabled={!bleStatus}
          style={styles.normalText}
          onPress ={()=>onGetHistory()}
        >
          L???ch s???
        </Button> */}
        <Button
          bg="red.400"
          p="3"
          w="45%"

          style={styles.normalText}
          isDisabled={!bleStatus}
          onPress ={()=>onDeleteHistory()}
        >
          X??a l???ch s???
        </Button>
      </HStack>
            
        
      </Center>
      </ScrollView> 
      

      

    </NativeBaseProvider>
  );
};

const styles = StyleSheet.create({
  normalText:{
    fontSize:18,
    fontWeight:"bold"
  },
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color:"#3b82f6"
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
});

export default (App);
