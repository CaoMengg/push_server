<?php

$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
$connection = socket_connect($socket, '127.0.0.1', '9090');

$arrData = array(
    'app_name' => 'baobao',
    'push_type' => 'xiaomi',
    'payload' => 'notify_id=6394&registration_id=l5nU6j%2B3FJ%2Fwi8I2sImbJVn%2BcSFaWWuR2hvnT9ZZ5BY%3D&time_to_live=604800000&title=%E6%B6%88%E6%81%AF%E6%8F%90%E9%86%92&description=%E4%BD%A0%E5%BE%97%E5%88%B0%E4%BA%86%E6%96%B0%E7%9A%84%E6%8A%B1%E6%8A%B1&notify_type=5&extra.notify_foreground=0&pass_through=0&payload=%7B%22pushId%22%3A1501839883521%2C%22type%22%3A%22interact%22%2C%22body%22%3A%22%5Cu4f60%5Cu5f97%5Cu5230%5Cu4e86%5Cu65b0%5Cu7684%5Cu62b1%5Cu62b1%22%2C%22badge%22%3A3%2C%22remind%22%3A1%2C%22settings%22%3A640%2C%22interact%22%3A%22baobao%22%2C%22from%22%3A1%2C%22info%22%3A%22%5B%5D%22%7D&restricted_package_name=cn.myhug.baobao',
);
$strMsg = json_encode($arrData);
$strMsg .= "\0";

$intLen = strlen( $strMsg );
if( socket_write( $socket, $strMsg, $intLen ) != $intLen ) {
    echo "send query fail\n";
    exit;
}
echo "send query succ\n";

$strResult = socket_read($socket, 10240);
var_dump( $strResult );
socket_close($socket); 
