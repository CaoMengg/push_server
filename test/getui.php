<?php

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
socket_set_option( $socket, SOL_SOCKET, SO_SNDTIMEO, array('sec'=>0, 'usec'=>100000) );
socket_set_option( $socket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>0, 'usec'=>100000) );
//$connection = socket_connect($socket, '127.0.0.1', '9090');
$connection = socket_connect($socket, '10.1.4.117', '9090');

$strContent = '{"pushId":1516776149985,"type":"msg","body":"\u62b1\u62b1\u5ba2\u670d\u53d1\u6765\u6d88\u606f","badge":3,"remind":1,"settings":640,"interact":"msg","from":3,"info":"[]"}';

// baobao
$arrPayload = array(
    'cid' => '0ba07ea4fc5436028cffc20bbd00b067',
    'requestid' => '1536909919732',
    'message' => array(
        'appkey' => 'ZOR7eYSDMi64JEpmOtfps9',
        'is_offline' => true,
        'msgtype' => 'transmission',
    ),
    'transmission' => array(
        'transmission_type' => false,
        'transmission_content' => $strContent,
        /*'notify' => array(
            'title' => 'oppo title',
            'content' => 'oppo content',
            'intent' => 'intent:#Intent;action=android.intent.action.oppopush;launchFlags=0x14000000;component=cn.myhug.baobao/.launcher.MainTabActivity;B._fbSourceApplicationHasBeenSet=true;S.data=' . urlencode($strContent) . ';end',
            //'intent' => 'mydata#Intent;action=android.intent.action.oppopush;launchFlags=0x10000000;component=cn.myhug.baobao/.launcher.MainTabActivity;end',
            'type' => '1',
        ),*/
    ),
);
$arrData = array(
    'app_name' => 'xsb',
    'push_type' => 'getui',
    'payload' => json_encode( $arrPayload ),
);

$strMsg = json_encode( $arrData );
$strMsg = '{"app_name":"xsb","push_type":"getui","token":"0ba07ea4fc5436028cffc20bbd00b067","payload":"{\"cid\":\"0ba07ea4fc5436028cffc20bbd00b067\",\"requestid\":1536909919732,\"message\":{\"appkey\":\"ZOR7eYSDMi64JEpmOtfps9\",\"is_offline\":true,\"msgtype\":\"transmission\"},\"transmission\":{\"transmission_type\":false,\"transmission_content\":\"{\\\"type\\\":\\\"zroom\\\",\\\"body\\\":\\\"\\\\u4f60\\\\u5173\\\\u6ce8\\\\u7684nyn\\\\u6b63\\\\u5728\\\\u76f4\\\\u64ad\\\",\\\"zId\\\":809}\"}}"}';
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
