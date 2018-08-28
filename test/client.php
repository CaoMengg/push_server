<?php

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
socket_set_option( $socket, SOL_SOCKET, SO_SNDTIMEO, array('sec'=>0, 'usec'=>100000) );
socket_set_option( $socket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>0, 'usec'=>100000) );
$connection = socket_connect($socket, '127.0.0.1', '9090');
//$connection = socket_connect($socket, '10.1.4.121', '9090');

// baobao xiaomi
/*$arrData = array(
    'app_name' => 'baobao',
    'push_type' => 'xiaomi',
    'payload' => 'notify_id=6394&registration_id=l5nU6j%2B3FJ%2Fwi8I2sImbJVn%2BcSFaWWuR2hvnT9ZZ5BY%3D&time_to_live=604800000&title=%E6%B6%88%E6%81%AF%E6%8F%90%E9%86%92&description=%E4%BD%A0%E5%BE%97%E5%88%B0%E4%BA%86%E6%96%B0%E7%9A%84%E6%8A%B1%E6%8A%B1&notify_type=5&extra.notify_foreground=0&pass_through=0&payload=%7B%22pushId%22%3A1501839883521%2C%22type%22%3A%22interact%22%2C%22body%22%3A%22%5Cu4f60%5Cu5f97%5Cu5230%5Cu4e86%5Cu65b0%5Cu7684%5Cu62b1%5Cu62b1%22%2C%22badge%22%3A3%2C%22remind%22%3A1%2C%22settings%22%3A640%2C%22interact%22%3A%22baobao%22%2C%22from%22%3A1%2C%22info%22%3A%22%5B%5D%22%7D&restricted_package_name=cn.myhug.baobao',
);*/

// baobao bbserver
$arrData = array(
    'app_name' => 'baobao',
    'push_type' => 'bbserver',
    'token' => 'http://10.1.1.119:7011/',
    //'token' => 'http://10.1.4.119:7000/',
    'payload' => 'iaVwVHlwZap3c3B1c2hfbXNnqHB1c2hEYXRhiaZwdXNoSWTPAAABZTxf4VmkdHlwZaNtc2ekYm9kebjmirHmirHlrqLmnI3lj5HmnaXmtojmga+lYmFkZ2UBpnJlbWluZAGoc2V0dGluZ3PNAoCoaW50ZXJhY3SjbXNnpGZyb20ApGluZm+iW12sX2JiX3NlcnZlcklwzgoBAXesX2JiX2NsaWVudEZkzQKfq19iYl92ZXJzaW9uAaxfYmJfY29tcHJlc3MAqF9iYl90eXBlA61fYmJfc3F1ZW5jZUlkAKdfYmJfY21kzQPr',
);

// avalon xiaomi
/*$arrData = array(
    'app_name' => 'avalon',
    'push_type' => 'xiaomi',
    'payload' => 'notify_id=6792&registration_id=L2nsKCk7b8qP3nvyiB8YijmuJ0%2F%2FtQ+oBPFKrKX83Es%3D&time_to_live=604800000&title=%E6%B6%88%E6%81%AF%E6%8F%90%E9%86%92&description=push_server%E5%8F%91%E6%9D%A5%E6%B6%88%E6%81%AF&notify_type=7&extra.notify_foreground=0&pass_through=0&payload=%7B%22pushId%22%3A%22PROC_START_TIME_MS%22%2C%22type%22%3A%22msg%22%2C%22body%22%3A%22abc%5Cu53d1%5Cu6765%5Cu6d88%5Cu606f%22%2C%22badge%22%3A1%2C%22settings%22%3A0%2C%22interact%22%3A%22%22%2C%22remind%22%3A1%2C%22from%22%3A1%2C%22app_name%22%3A%22app_avalon%22%7D&restricted_package_name=cn.myhug.avalon',
);*/

// avalon apns
/*$arrData = array(
    'app_name' => 'avalon',
    'push_type' => 'apns',
    'token' => 'febbb4a2f85ff01923cef5bf3ba2fa9f52ce5183b2479e6027d618f313453407',
    'payload' => '{"aps":{"alert":{"body":"push_server\u53d1\u6765\u6d88\u606f","launch-image":"icon_120"},"sound":"default","badge":1},"avalon":{"app_name":"app_avalon","type":"msg"}}',
);*/

// avalon apns sandbox
/*$arrData = array(
    'app_name' => 'avalon_sandbox',
    'push_type' => 'apns',
    'token' => '91a66e4cbcf863d49438767b948b6c16c36d3db14f53a586c7c4c9ac5c90c895',
    'payload' => '{"aps":{"alert":{"body":"abc\u53d1\u6765\u6d88\u606f","launch-image":"icon_120"},"sound":"default","badge":2},"avalon":{"app_name":"app_avalon","type":"msg"}}',
);*/

// baobao voip
/*$arrData = array(
    'app_name' => 'baobao_voip',
    'push_type' => 'apns',
    'token' => 'bfaab846c4dc4749c8cc2ef5baf71cff5cb717199351b3ad29a2a011f702db05',
    'payload' => '{"aps":{"alert":{"body":"__call--","launch-image":"icon_120"},"badge":3,"mutable-content":1,"sound":"default"},"baobao":{"type":"call","info":"[]"},"picUrl":"http:\/\/pws.myhug.cn\/npic\/p\/9\/ff58392b9310bee81c1069ec4ca9f1ce44598dae2f3f25"}',
);*/

// coneop getui
/*$arrData = array(
    'app_name' => 'baobao',
    'push_type' => 'getui',
    'token' => '5121afee7d611edee3cd57a9edaeb11e',
    'payload' => '{"action":"pushMessageToSingleAction","clientData":"CgASC3B1c2htZXNzYWdlGgAiFlNEY0s3UXVWRUlBUVdUcXJQRzBsUUEqFmJZc0F2RzRFdXJBcVNqNDc4WTJvSTcyADoKCgASABoAIgItMUIKCAEQABiuTpAKAEIcCK5OEAMYZOIIAOoIBgoAEgAaAPAIAPgIZJAKAEIHCGQQB5AKAA==","transmissionContent":"{\"pushId\":\"PROC_START_TIME_MS\",\"type\":\"msg\",\"body\":\"\\u6709\\u4eba\\u7ed9\\u4f60\\u53d1\\u6765\\u6d88\\u606f\",\"badge\":0,\"settings\":0,\"interact\":\"\",\"remind\":1,\"from\":3}","isOffline":false,"offlineExpireTime":604800,"pushNetWorkType":0,"appkey":"SDcK7QuVEIAQWTqrPG0lQA","appId":"bYsAvG4EurAqSj478Y2oI7","clientId":"5121afee7d611edee3cd57a9edaeb11e","alias":null,"type":2,"pushType":"TransmissionMsg"}',
);*/




$strMsg = json_encode($arrData);
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
