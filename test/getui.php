<?php

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
socket_set_option( $socket, SOL_SOCKET, SO_SNDTIMEO, array('sec'=>0, 'usec'=>100000) );
socket_set_option( $socket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>0, 'usec'=>100000) );
$connection = socket_connect($socket, '127.0.0.1', '9090');
//$connection = socket_connect($socket, '10.1.4.121', '9090');

$strContent = '{"pushId":1516776149985,"type":"msg","body":"\u62b1\u62b1\u5ba2\u670d\u53d1\u6765\u6d88\u606f","badge":3,"remind":1,"settings":640,"interact":"msg","from":3,"info":"[]"}';

// baobao
$arrPayload = array(
    //'cid' => '9dabd538e18e5e3772c399f240a09f83',
    'cid' => '6847a036245978ad72097d0c5802baa5',
    'requestid' => '01234567890'. rand(),
    'message' => array(
        //'appkey' => '5yleRscqsd5gXowkOOxYjA',
        'appkey' => '7rVNopw0N374UJAueXEfW1',
        'is_offline' => true,
        'msgtype' => 'transmission',
    ),
    'transmission' => array(
        'transmission_type' => false,
        //'transmission_content' => '{"action":"pushMessageToSingleAction","clientData":"CgASC3B1c2htZXNzYWdlGgAiFjdyVk5vcHcwTjM3NFVKQXVlWEVmVzEqFnJpNUxhUExieHA4QktBTW1CN3VhYjcyADoKCgASABoAIgItMUIKCAEQABiuTpAKAEIcCK5OEAMYZOIIAOoIBgoAEgAaAPAIAPgIZJAKAEIHCGQQB5AKAA==","transmissionContent":"{\"pushId\":1531823595501,\"type\":\"msg\",\"body\":\"\\u91ca\\u653e\\u53d1\\u6765\\u6d88\\u606f\",\"badge\":1,\"remind\":1,\"settings\":640,\"interact\":\"msg\",\"from\":3,\"info\":\"[]\"}","isOffline":false,"offlineExpireTime":604800,"pushNetWorkType":0,"appkey":"7rVNopw0N374UJAueXEfW1","appId":"ri5LaPLbxp8BKAMmB7uab7","clientId":"e918738cb7d27e1fd831f729c5b18e5d","alias":null,"type":2,"pushType":"TransmissionMsg"}',
        'transmission_content' => $strContent,
        //"duration_begin" => "2018-01-24 11:40:00",
        //"duration_end" => "2018-01-24 23:40:00",
        'notify' => array(
            'title' => 'oppo title',
            'content' => 'oppo content',
            'intent' => 'intent:#Intent;action=android.intent.action.oppopush;launchFlags=0x14000000;component=cn.myhug.baobao/.launcher.MainTabActivity;B._fbSourceApplicationHasBeenSet=true;S.data=' . urlencode($strContent) . ';end',
            //'intent' => 'mydata#Intent;action=android.intent.action.oppopush;launchFlags=0x10000000;component=cn.myhug.baobao/.launcher.MainTabActivity;end',
            'type' => '1',
        ),
    ),
);
$arrData = array(
    'app_name' => 'baobao',
    'push_type' => 'getui',
    'payload' => json_encode( $arrPayload ),
);

$strMsg = json_encode( $arrData );
$strMsg .= "\0";

$intLen = strlen( $strMsg );
if( socket_write( $socket, $strMsg, $intLen ) != $intLen ) {
    echo "send query fail\n";
    exit;
}
echo "send query succ\n";

$strResult = socket_read($socket, 10240);
if( empty( $strResult ) ) {
    echo "res is empty\n";
} else {
    $arrResult = json_decode( $strResult, true );
    if( $arrResult == false ) {
        echo "res is not json\n";
    } else {
        var_dump( $arrResult );
    }
}

socket_close($socket); 
